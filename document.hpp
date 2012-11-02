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

// This file declares abstractions for document data.

#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include <string>
#include <vector>

// An abstraction for a document.
class Document {
 public:
  // Simple structure representing the size in pixels of a page.
  struct PageSize {
    int Width;
    int Height;

    PageSize(int width, int height)
        : Width(width), Height(height) {
    }
  };

  // An item in a outline. An item may contain further children items.
  class OutlineItem {
   public:
    virtual ~OutlineItem();

    // Returns the display text of this item.
    const std::string &GetTitle() const;
    // Returns the number of children items contained within this item.
    int GetChildrenCount() const;
    // Returns a const pointer to the i-th child of this item.
    const OutlineItem *GetChild(int i);

   protected:
    std::string _title;
    std::vector<OutlineItem *> _children;
  };

  virtual ~Document();
  // Returns the number of pages in the document.
  virtual int GetPageCount() = 0;
  // Returns the size of a page, in pixels.
  virtual const PageSize GetPageSize(int page) = 0;
  // Renders the given page to a buffer. Page numbers are 0-based. zoom gives
  // the zoom ratio, e.g., 1.5 = 150%. rotation is the desired rotation in
  // clockwise degrees. depth is the bit-depth in bytes of the buffer, so a
  // 32bbp buffer should have a depth of 4.  Returns a newly allocated buffer
  // containing the rendered page. Caller owns returned buffer. If an error
  // occurred during the rendering, NULL is returned.
  virtual void *Render(int depth, int page, float zoom, int rotation) = 0;
  // Returns the outline of this document. The returned item represents the
  // top-level element in the outline, and is owned by the caller.
  virtual const OutlineItem *GetOutline() = 0;
  // Returns the page number referred to by an outline item.
  virtual int Lookup(const OutlineItem &item) = 0;
};

#endif

