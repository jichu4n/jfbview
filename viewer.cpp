/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012 Chuan Ji <jichuan89@gmail.com>                        *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *   http://www.apache.org/licenses/LICENSE-2.0                              *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// This file defines a Viewer class, which maintains state for rendering a
// Document page on top of Framebuffer.

#include "viewer.hpp"
#include "document.hpp"
#include "framebuffer.hpp"
#include <pthread.h>
#include <cassert>
#include <unistd.h>
#include <algorithm>

const float Viewer::MAX_ZOOM = 10.0f;
const float Viewer::MIN_ZOOM = 0.1f;

namespace {

// A PixelWriter that writes pixel values to a in-memory buffer. Each pixel is
// stored as three consecutive ints representing the r, g, and b values.
class BufferPixelWriter: public Document::PixelWriter {
 public:
   // Constructs a BufferPixel writer with the given settings. buffer_width
   // gives the number of pixels in a row in the buffer. buffer is the target
   // buffer.
  BufferPixelWriter(int buffer_width, int *buffer)
      : _buffer_width(buffer_width), _buffer(buffer) {
  }
  // See PixelWriter.
  virtual void Write(int x, int y, int r, int g, int b) {
    int *p = _buffer + (_buffer_width * y + x) * 3;
    p[0] = r;
    p[1] = g;
    p[2] = b;
  }
 private:
  // Number of pixels in a line.
  int _buffer_width;
  // The destination buffer.
  int *_buffer;
};

// Argument passed to BlitWorker.
struct BlitWorkerArg {
  // Source buffer. Each pixel is represented using three ints (r, g, b).
  int *Buffer;
  // The framebuffer to render to.
  Framebuffer *Fb;
  // The pixel row in the buffer to start rendering from.
  int BufferStartY;
  // The first pixel column in the buffer to render.
  int BufferStartX;
  // The pixel row in the framebuffer to start rendering from.
  int FbStartRow;
  // The last row + 1 to render.
  int BufferEndY;
  // The width (in pixels) of a line in the buffer.
  int BufferWidth;
};

// A worker thread that writes a rectangular area of a pixel buffer to a
// framebuffer device. This is passed to pthread_create. The rectangular area is
// defined by the the argument, which should be of type BlitWorkerArg.
void *BlitWorker(void *_arg) {
  BlitWorkerArg *arg = reinterpret_cast<BlitWorkerArg *>(_arg);
  const int width = arg->Fb->GetSize().Width;
  const int num_rows = arg->BufferEndY - arg->BufferStartY;
  for (int y = 0; y < num_rows; ++y) {
    for (int x = 0; x < width; ++x) {
      int *p = arg->Buffer + (((arg->BufferStartY + y) * arg->BufferWidth) +
                               (arg->BufferStartX + x)) * 3;
      arg->Fb->WritePixel(x, y + arg->FbStartRow, p[0], p[1], p[2]);
    }
  }
  return NULL;
}

}  // namespace


Viewer::Viewer(Document *doc, Framebuffer *fb, const Viewer::Config &config,
               int render_cache_size)
    : _doc(doc), _fb(fb), _config(config),
      _render_cache(this, render_cache_size) {
  assert(_doc != NULL);
  assert(_fb != NULL);
}

Viewer::~Viewer() {
}

void Viewer::Render() {
  // 1. Process config.
  int page = std::max(0, std::min(_doc->GetPageCount() - 1, _config.Page));
  float zoom = _config.Zoom;
  if (zoom == ZOOM_TO_WIDTH) {
    zoom = static_cast<float>(_fb->GetSize().Width) /
           static_cast<float>(
               _doc->GetPageSize(page, 1.0f, _config.Rotation).Width);
  } else if (zoom == ZOOM_TO_FIT) {
    const Framebuffer::ScreenSize &screen_size = _fb->GetSize();
    const Document::PageSize &page_size =
        _doc->GetPageSize(page, 1.0f, _config.Rotation);
    zoom = std::min(static_cast<float>(screen_size.Width) /
                        static_cast<float>(page_size.Width),
                    static_cast<float>(screen_size.Height) /
                        static_cast<float>(page_size.Height));
  }
  assert(zoom >= 0.0f);
  zoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, zoom));
  const Framebuffer::ScreenSize &screen_size = _fb->GetSize();
  const Document::PageSize &page_size =
      _doc->GetPageSize(page, zoom, _config.Rotation);
  int x_offset = std::max(0, std::min(page_size.Width - screen_size.Width - 1,
                                      _config.XOffset));
  int y_offset = std::max(0, std::min(page_size.Height - screen_size.Height - 1,
                                      _config.YOffset));

  // 2. Render page to buffer.
  int *buffer = _render_cache.Get(RenderCacheKey(page, zoom, _config.Rotation));

  // 3. Blit buffer to framebuffer.
  int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
  int num_rows_per_thread = screen_size.Height / num_threads;
  pthread_t *threads = new pthread_t[num_threads];
  BlitWorkerArg *args = new BlitWorkerArg[num_threads];
  for (int i = 0; i < num_threads; ++i) {
    args[i].Buffer = buffer;
    args[i].Fb = _fb;
    args[i].BufferStartY = y_offset + i * num_rows_per_thread;
    args[i].BufferStartX = x_offset;
    args[i].FbStartRow = i * num_rows_per_thread;
    args[i].BufferEndY =
        (i == num_threads - 1) ? screen_size.Height :
                                 (i + 1) * num_rows_per_thread;
    args[i].BufferWidth = page_size.Width;

    pthread_create(&(threads[i]), NULL, &BlitWorker, &(args[i]));
  }
  for (int i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }
  delete []args;
  delete []threads;
}

Viewer::RenderCache::RenderCache(Viewer *parent, int size)
    : Cache<RenderCacheKey, int *>(size), _parent(parent) {
}

Viewer::RenderCache::~RenderCache() {
  Clear();
}

int *Viewer::RenderCache::Load(const RenderCacheKey &key) {
  const Document::PageSize &page_size =
      _parent->_doc->GetPageSize(key.Page, key.Zoom, key.Rotation);
  int buffer_size = page_size.Width * page_size.Height;

  int *buffer = new int[buffer_size * 3];
  BufferPixelWriter writer(page_size.Width, buffer);
  _parent->_doc->Render(&writer, key.Page, key.Zoom, key.Rotation);

  return buffer;
}

void Viewer::RenderCache::Discard(const RenderCacheKey &key, int * &value) {
  delete []value;
}

#include "pdf_document.hpp"

int main(int argc, char *argv[]) {
  assert(argc == 2);
  Document *doc = PDFDocument::Open(argv[1]);
  assert(doc != NULL);
  Framebuffer *fb = Framebuffer::Open();
  assert(fb != NULL);
  Viewer *viewer = new Viewer(doc, fb);
  viewer->Render();

  delete viewer;
  delete fb;
  delete doc;
}

