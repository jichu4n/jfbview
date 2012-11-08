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

#ifndef VIEWER_HPP
#define VIEWER_HPP

#include "cache.hpp"

class Document;
class Framebuffer;
class PixelBuffer;

class Viewer {
 public:
  // Default number of rendered pages to keep in cache.
  enum { DEFAULT_RENDER_CACHE_SIZE = 5 };

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

    // The zoom ratio, or ZOOM_*.
    float Zoom;
    // Rotation of the document, in clockwise degrees.
    int Rotation;

    // Number of screen pixels from top of page to top of displayed view.
    int XOffset;
    // Number of screen pixels from left of page to left of displayed view.
    int YOffset;

    State(int page = 0, float zoom = ZOOM_TO_WIDTH, int rotation = 0,
           int x_offset = 0, int y_offset = 0)
        : Page(page), Zoom(zoom), Rotation(rotation),
          XOffset(x_offset), YOffset(y_offset) {
    }
  };

  // Constructs a new Viewer object. Does not take ownership of the document or
  // the framebuffer object.
  Viewer(Document *doc, Framebuffer *fb, const State &state = State(),
         int render_cache_size = DEFAULT_RENDER_CACHE_SIZE);
  virtual ~Viewer();

  // Renders the present view to the framebuffer.
  void Render();

  // Returns the current settings.
  State GetState() const;
  // Sets the current settings. Will use minimum and maximum legal values to
  // replace illegal values.
  void SetState(const State &pan);

  // Returns whether the current view is at the bottom of the page.
  bool AtPageBottom() const;
  // Returns whether the current view is at the top of the page.
  bool AtPageTop() const;

 private:
  // The current document.
  Document *_doc;
  // The framebuffer device.
  Framebuffer *_fb;
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
    inline bool operator < (const RenderCacheKey &other) const {
      if (Page != other.Page) {
        return Page < other.Page;
      }
      if (Zoom != other.Zoom) {
        return Zoom < other.Zoom;
      }
      return Rotation < other.Rotation;
    }
  };
  // Render cache class.
  class RenderCache: public Cache<RenderCacheKey, PixelBuffer *> {
   public:
    RenderCache(Viewer *parent, int size);
    virtual ~RenderCache();
   protected:
    virtual PixelBuffer *Load(const RenderCacheKey &key);
    virtual void Discard(const RenderCacheKey &key, PixelBuffer * &value);
   private:
    Viewer *_parent;
  };
  // Render cache.
  RenderCache _render_cache;
};

#endif

