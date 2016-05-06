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

#ifndef VIEWER_HPP
#define VIEWER_HPP

#include "cache.hpp"

class Document;
class Framebuffer;
class PixelBuffer;

class Viewer {
 public:
  // Default number of rendered pages to keep in cache.
  enum { DEFAULT_RENDER_CACHE_SIZE = 8 };

  // Zoom modes.
  enum {
    // Automatically zoom to fit current page.
    ZOOM_TO_FIT = -3,
    // Automatically zoom to fit current page width.
    ZOOM_TO_WIDTH = -4,
  };

  // Maximum zoom ratio.
  static const float MAX_ZOOM;
  // Minimum zoom ratio.
  static const float MIN_ZOOM;

  // A structure representing zoom information.
  struct State {
    // The displayed page.
    int Page;
    // The total number of pages in the document. This is written by Render()
    // and is ignored by Render() itself.
    int NumPages;

    // The zoom ratio, or ZOOM_*.
    float Zoom;
    // If Zoom is ZOOM_*, this gives the actual zoom value. This is written by
    // Render() and is ignored by Render() itself.
    float ActualZoom;
    // Rotation of the document, in clockwise degrees.
    int Rotation;

    // Number of screen pixels from top of page to top of displayed view.
    int XOffset;
    // Number of screen pixels from left of page to left of displayed view.
    int YOffset;

    // Width of current page (after zoom and rotation). This is written by
    // Render(), and is ignored by Render() itself.
    int PageWidth;
    // Height of current page (after zoom and rotation). This is written by
    // Render(), and is ignored by Render() itself.
    int PageHeight;
    // Width of framebuffer. This is written by Render(), and is ignored by
    // Render() itself.
    int ScreenWidth;
    // Height of framebuffer. This is written by Render(), and is ignored by
    // Render() itself.
    int ScreenHeight;

    State(int page = 0, float zoom = ZOOM_TO_WIDTH, int rotation = 0,
           int x_offset = 0, int y_offset = 0)
        : Page(page), Zoom(zoom), Rotation(rotation),
          XOffset(x_offset), YOffset(y_offset) {
    }
  };

  // Constructs a new Viewer object. Does not take ownership of the document or
  // the framebuffer object.
  Viewer(
      Document* doc,
      Framebuffer* fb,
      const State& state = State(),
      int render_cache_size = DEFAULT_RENDER_CACHE_SIZE);
  virtual ~Viewer();

  // Renders the present view to the framebuffer.
  void Render();

  // Stores the current state in the given pointer. Must be called AFTER at
  // least one call to Render().
  void GetState(State* state) const;
  // Sets the current settings. Will use minimum and maximum legal values to
  // replace illegal values. Has no effect until Render() is called.
  void SetState(const State& state);

 private:
  // The current document.
  Document* _doc;
  // The framebuffer device.
  Framebuffer* _fb;
  // Settings.
  State _state;

  // Key to the render cache.
  struct RenderCacheKey {
    // Page number, starting from 0.
    int Page;
    // Zoom ratio at which the buffer was rendered. This must be the actual
    // ratio, and NOT one of the ZOOM_* constants.
    float Zoom;
    // Rotation in clockwise degrees.
    int Rotation;

    RenderCacheKey(int page, float zoom, int rotation)
        : Page(page), Zoom(zoom), Rotation(rotation) {
    }

    // This is required as this class will be inserted into a map.
    bool operator < (const RenderCacheKey& other) const;
  };
  // Render cache class.
  class RenderCache : public Cache<RenderCacheKey, PixelBuffer*> {
   public:
    RenderCache(Viewer* parent, int size);
    virtual ~RenderCache();
   protected:
    PixelBuffer* Load(const RenderCacheKey& key) override;
    void Discard(
        const RenderCacheKey& key, PixelBuffer* const& value) override;
   private:
    Viewer* _parent;
  };
  // Render cache.
  RenderCache _render_cache;
};

#endif

