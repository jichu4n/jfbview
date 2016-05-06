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

// This file declares abstractions for document data.

#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include <string>
#include <memory>
#include <vector>

// An abstraction for a document.
class Document {
 public:
  // Simple structure representing the size in pixels of a page.
  struct PageSize {
    int Width;
    int Height;

    explicit PageSize(int width = -1, int height = -1)
        : Width(width), Height(height) {
    }
  };

  // An interface for a callback that stores a pixel in a memory buffer.
  class PixelWriter {
   public:
    // Writes a pixel value (r, g, b) to position (x, y). It is important that
    // Write be thread-safe when called with different (x, y).
    virtual void Write(int x, int y, int r, int g, int b) = 0;
  };

  // An item in a outline. An item may contain further children items.
  class OutlineItem {
   public:
    virtual ~OutlineItem();

    // Returns the display text of this item.
    const std::string& GetTitle() const;
    // Returns the number of children items contained within this item.
    int GetNumChildren() const;
    // Returns a const pointer to the i-th child of this item.
    const OutlineItem* GetChild(int i) const;

   protected:
    std::string _title;
    std::vector<std::unique_ptr<OutlineItem>> _children;
  };

  // A text search hit.
  struct SearchHit {
    // The page number where the search hit occurred.
    int Page;
    // Context text.
    std::string ContextText;
    // Position of search string in context text.
    int SearchStringPosition;

    explicit SearchHit(
        int page = -1,
        const std::string& context_text = std::string(),
        int search_string_position = -1)
        : Page(page),
          ContextText(context_text),
          SearchStringPosition(search_string_position) {
    }
  };
  struct SearchResult {
    // The search string.
    std::string SearchString;
    // Last page that was searched.
    int LastSearchedPage;
    // Hits.
    std::vector<SearchHit> SearchHits;
  };

  virtual ~Document();

  // Returns the number of pages in the document.
  virtual int GetNumPages() = 0;

  // Returns the size of a page, in pixels. zoom gives the zoom ratio as a
  // fraction, e.g., 1.5 = 150%. rotation is the desired rotation in clockwise
  // degrees.
  virtual const PageSize GetPageSize(
      int page, float zoom = 1.0f, int rotation = 0) = 0;

  // Renders the given page to a buffer. Page numbers are 0-based. zoom gives
  // the zoom ratio as a fraction, e.g., 1.5 = 150%. rotation is the desired
  // rotation in clockwise degrees. For every rendered pixel, pw will be invoked
  // to store that pixel value somewhere.
  virtual void Render(PixelWriter* pw, int page, float zoom, int rotation) = 0;

  // Returns the outline of this document. The returned item represents the
  // top-level element in the outline, and is owned by the caller. If the
  // document does not have an outline, return nullptr.
  virtual const OutlineItem* GetOutline() = 0;

  // Returns the page number referred to by an outline item. If not available,
  // returns -1.
  virtual int Lookup(const OutlineItem* item) = 0;

  // Searches the text of the document. Will return up to max_num_search_hits
  // search hits starting from the given page.
  SearchResult Search(
      const std::string& search_string,
      int start_page,
      int context_length,
      int max_num_search_hits);

 protected:
  // Performs a text search on a given page.
  virtual std::vector<SearchHit> SearchOnPage(
      const std::string& search_string, int page, int context_length) = 0;
};

#endif

