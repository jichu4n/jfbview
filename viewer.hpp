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

class Viewer {
 public:
  // Zoom modes.
  enum ZoomMode {
    // A fixed zoom ratio, e.g., 150%.
    ZOOM_FIXED,
    // Automatically zoom to fit current page.
    ZOOM_TO_FIT,
    // Automatically zoom to fit current page width.
    ZOOM_TO_WIDTH,
  };
  // A structure representing zoom information.
  struct ZoomInfo {
    // The zoom mode.
    ZoomMode Mode;
    // If Mode is ZOOM_FIXED, this gives the zoom ratio as a decimal.
    float Ratio;
  };

  // Constructs a new Viewer object. Does not take ownership of the document or
  // the framebuffer object.
  Viewer(Document *doc, Framebuffer *fb);
  virtual ~Viewer();

  // Renders the present view to the framebuffer.
  void Render();

  // Returns the currently displayed page number. Page numbers start from 0.
  int GetPage() const;
  // Sets the currently displayed page number. Page numbers start from 0. If the
  // page number is invalid, this method does nothing and returns false.
  // Otherwise returns true.
  bool SetPage(int page);

  // Returns the current zoom settings.
  ZoomInfo GetZoomInfo() const;
  // Sets the current zoom settings. Tries to preserve the center of the current
  // view.
  void SetZoomInfo();
};

#endif

