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
#warning remove
#include <cstdio>
#include <pthread.h>
#include <cassert>
#include <unistd.h>
#include <algorithm>

const float Viewer::MAX_ZOOM = 10.0f;
const float Viewer::MIN_ZOOM = 0.1f;

namespace {

// A PixelWriter that writes pixel values to a in-memory buffer. Each pixel is
// stored as three consecutive ints representing the r, g, and b values.
class PixelBufferWriter: public Document::PixelWriter {
 public:
   // Constructs a BufferPixel writer with the given settings. buffer_width
   // gives the number of pixels in a row in the buffer. buffer is the target
   // buffer.
  PixelBufferWriter(PixelBuffer *buffer)
      : _buffer(buffer) {
  }
  // See PixelWriter.
  virtual void Write(int x, int y, int r, int g, int b) {
    _buffer->WritePixel(x, y, r, g, b);
  }
 private:
  // The destination buffer.
  PixelBuffer *_buffer;
};

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
    const PixelBuffer::Size &screen_size = _fb->GetSize();
    const Document::PageSize &page_size =
        _doc->GetPageSize(page, 1.0f, _config.Rotation);
    zoom = std::min(static_cast<float>(screen_size.Width) /
                        static_cast<float>(page_size.Width),
                    static_cast<float>(screen_size.Height) /
                        static_cast<float>(page_size.Height));
  }
  assert(zoom >= 0.0f);
  zoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, zoom));
  const PixelBuffer::Size &screen_size = _fb->GetSize();
  const Document::PageSize &page_size =
      _doc->GetPageSize(page, zoom, _config.Rotation);
  PixelBuffer::Rect src_rect;
  src_rect.X = std::max(0, std::min(page_size.Width - screen_size.Width - 1,
                                    _config.XOffset));
  src_rect.Y = std::max(0, std::min(page_size.Height - screen_size.Height - 1,
                                    _config.YOffset));
  src_rect.Width = std::min(screen_size.Width, page_size.Width - src_rect.X);
  src_rect.Height = std::min(screen_size.Height, page_size.Height - src_rect.Y);

  // 2. Render page to buffer.
  PixelBuffer *buffer =
      _render_cache.Get(RenderCacheKey(page, zoom, _config.Rotation));

  // 3. Blit buffer.
  _fb->Render(*buffer, src_rect);
}

Viewer::RenderCache::RenderCache(Viewer *parent, int size)
    : Cache<RenderCacheKey, PixelBuffer *>(size), _parent(parent) {
}

Viewer::RenderCache::~RenderCache() {
  Clear();
}

PixelBuffer *Viewer::RenderCache::Load(const RenderCacheKey &key) {
  printf("Load %d\n", key.Page);
  const Document::PageSize &page_size =
      _parent->_doc->GetPageSize(key.Page, key.Zoom, key.Rotation);

  PixelBuffer *buffer = _parent->_fb->NewPixelBuffer(PixelBuffer::Size(
      page_size.Width, page_size.Height));
  PixelBufferWriter writer(buffer);
  _parent->_doc->Render(&writer, key.Page, key.Zoom, key.Rotation);

  return buffer;
}

void Viewer::RenderCache::Discard(const RenderCacheKey &key,
                                  PixelBuffer * &value) {
  delete value;
}

#include "pdf_document.hpp"

int main(int argc, char *argv[]) {
  assert(argc == 2);
  Document *doc = PDFDocument::Open(argv[1]);
  assert(doc != NULL);
  Framebuffer *fb = Framebuffer::Open();
  assert(fb != NULL);
  Viewer *viewer = new Viewer(doc, fb);
  for (int i = 0; i < 1000; ++i) {
    viewer->Render();
  }

  delete viewer;
  delete fb;
  delete doc;
}

