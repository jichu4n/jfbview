/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012-2014 Chuan Ji <ji@chu4n.com>                          *
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
#include <unistd.h>
#include <cassert>
#include <cmath>
#include <algorithm>
#include "document.hpp"
#include "framebuffer.hpp"

const float Viewer::MAX_ZOOM = 10.0f;
const float Viewer::MIN_ZOOM = 0.1f;

namespace {

// A PixelWriter that writes pixel values to a in-memory buffer. Each pixel is
// stored as three consecutive ints representing the r, g, and b values.
class PixelBufferWriter : public Document::PixelWriter {
 public:
  // Constructs a BufferPixel writer with the given settings. buffer_width
  // gives the number of pixels in a row in the buffer. buffer is the target
  // buffer.
  explicit PixelBufferWriter(PixelBuffer* buffer)
      : _buffer(buffer) {
  }
  // See PixelWriter.
  void Write(int x, int y, int r, int g, int b) override {
    _buffer->WritePixel(x, y, r, g, b);
  }
 private:
  // The destination buffer.
  PixelBuffer* _buffer;
};

}  // namespace


Viewer::Viewer(Document* doc, Framebuffer* fb, const Viewer::State& state,
               int render_cache_size)
    : _doc(doc), _fb(fb), _state(state),
      _render_cache(this, render_cache_size) {
  assert(_doc != nullptr);
  assert(_fb != nullptr);
}

Viewer::~Viewer() {
}

void Viewer::Render() {
  // 1. Process state.
  int page = std::max(0, std::min(_doc->GetNumPages() - 1, _state.Page));
  float zoom = _state.Zoom;
  if (zoom == ZOOM_TO_WIDTH) {
    zoom = static_cast<float>(_fb->GetSize().Width) /
           static_cast<float>(
               _doc->GetPageSize(page, 1.0f, _state.Rotation).Width);
  } else if (zoom == ZOOM_TO_FIT) {
    const PixelBuffer::Size& screen_size = _fb->GetSize();
    const Document::PageSize& page_size =
        _doc->GetPageSize(page, 1.0f, _state.Rotation);
    zoom = std::min(static_cast<float>(screen_size.Width) /
                        static_cast<float>(page_size.Width),
                    static_cast<float>(screen_size.Height) /
                        static_cast<float>(page_size.Height));
  }
  assert(zoom >= 0.0f);
  zoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, zoom));

  // 2. Render page to buffer.
  PixelBuffer* buffer =
      _render_cache.Get(RenderCacheKey(page, zoom, _state.Rotation));

  // 3. Compute the area actually visible on screen.
  const PixelBuffer::Size& screen_size = _fb->GetSize(),
                          &page_size = buffer->GetSize();
  PixelBuffer::Rect src_rect;
  src_rect.X = std::max(0, std::min(page_size.Width - screen_size.Width - 1,
                                    _state.XOffset));
  src_rect.Y = std::max(0, std::min(page_size.Height - screen_size.Height - 1,
                                    _state.YOffset));
  src_rect.Width = std::min(screen_size.Width, page_size.Width - src_rect.X);
  src_rect.Height = std::min(screen_size.Height, page_size.Height - src_rect.Y);

  // 4. Blit visible area to framebuffer.
  _fb->Render(*buffer, src_rect);

  // 5. Store corrected state.
  _state.Page = page;
  _state.NumPages = _doc->GetNumPages();
  if ((_state.Zoom != ZOOM_TO_WIDTH) && (_state.Zoom != ZOOM_TO_FIT)) {
    _state.Zoom = zoom;
  }
  _state.ActualZoom = zoom;
  _state.XOffset = src_rect.X;
  _state.YOffset = src_rect.Y;
  _state.PageWidth = page_size.Width;
  _state.PageHeight = page_size.Height;
  _state.ScreenWidth = screen_size.Width;
  _state.ScreenHeight = screen_size.Height;

  // 6. Preload.
  if ((_render_cache.GetSize() > 1) && (page < _doc->GetNumPages() - 1)) {
    _render_cache.Prepare(RenderCacheKey(page + 1, zoom, _state.Rotation));
  }
}

void Viewer::GetState(Viewer::State* state) const {
  state->Page = _state.Page;
  state->NumPages = _state.NumPages;
  state->Zoom = _state.Zoom;
  state->ActualZoom = _state.ActualZoom;
  state->Rotation = _state.Rotation;
  state->XOffset = _state.XOffset;
  state->YOffset = _state.YOffset;
  state->PageWidth = _state.PageWidth;
  state->PageHeight = _state.PageHeight;
  state->ScreenWidth = _state.ScreenWidth;
  state->ScreenHeight = _state.ScreenHeight;
}

void Viewer::SetState(const State& state) {
  _state = state;
}


bool Viewer::RenderCacheKey::operator < (
    const Viewer::RenderCacheKey& other) const {
  if (Page != other.Page) {
    return Page < other.Page;
  }
  const int rotation_mod = Rotation % 360,
            other_rotation_mod = other.Rotation % 360;
  if (rotation_mod != other_rotation_mod) {
    return rotation_mod < other_rotation_mod;
  }
  if (fabs(Zoom / other.Zoom - 1.0f) >= 0.1f) {
    return Zoom < other.Zoom;
  }
  return false;
}

Viewer::RenderCache::RenderCache(Viewer* parent, int size)
    : Cache<RenderCacheKey, PixelBuffer*>(size), _parent(parent) {
}

Viewer::RenderCache::~RenderCache() {
  Clear();
}

PixelBuffer* Viewer::RenderCache::Load(const RenderCacheKey& key) {
  const Document::PageSize& page_size =
      _parent->_doc->GetPageSize(key.Page, key.Zoom, key.Rotation);

  PixelBuffer* buffer = _parent->_fb->NewPixelBuffer(PixelBuffer::Size(
      page_size.Width, page_size.Height));
  PixelBufferWriter writer(buffer);
  _parent->_doc->Render(&writer, key.Page, key.Zoom, key.Rotation);

  return buffer;
}

void Viewer::RenderCache::Discard(
    const RenderCacheKey& key, PixelBuffer* const& value) {
  delete value;
}

