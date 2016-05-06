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

// This file declares PDFDocument, an implementation of the Document abstraction
// using MuPDF.

#ifndef PDF_DOCUMENT_HPP
#define PDF_DOCUMENT_HPP

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "cache.hpp"
#include "document.hpp"
// HACK ALERT: MuPDF is in written C, and due to a stupid incompatibility with
// struct declaration syntax between C and C++, including <mupdf/pdf.h> causes a
// link error. So, what we do is the following:
//   1. Only include <mupdf/fitz.h> in pdf_document.hpp, so any other files that
//      include this header will not get <mupdf/pdf.h>. Types defined in
//      <mupdf/pdf.h> will be forward-declared in pdf_document.hpp.
//   2. pdf_document.cpp will include <mupdf/pdf.h> first before including
//      pdf_document.hpp. pdf_document.hpp will detect this fact and not
//      forward-declare the types defined there, as doing so will cause a
//      compile error.
extern "C" {
#include "mupdf/fitz.h"
#ifndef MUPDF_PDF_H
  struct pdf_document;
  struct pdf_page;
#endif
}

// Document implementation using MuPDF.
class PDFDocument : public Document {
 public:
  // Default size of page cache.
  enum { DEFAULT_PAGE_CACHE_SIZE = 5 };

  virtual ~PDFDocument();
  // Factory method to construct an instance of PDFDocument. path gives the path
  // to a PDF file. page_cache_size specifies the maximum number of pages to
  // store in memory. Returns nullptr if the file cannot be opened.
  static PDFDocument* Open(
      const std::string& path, int page_cache_size = DEFAULT_PAGE_CACHE_SIZE);
  // See Document.
  int GetNumPages() override;
  // See Document.
  const PageSize GetPageSize(int page, float zoom, int rotation) override;
  // See Document. Thread-safe.
  void Render(PixelWriter* pw, int page, float zoom, int rotation) override;
  // See Document.
  const OutlineItem* GetOutline() override;
  // See Document.
  int Lookup(const OutlineItem* item) override;

  // Returns the text content of a page, using line_sep to separate lines.
  std::string GetPageText(int page, int line_sep = '\n');

 protected:
  std::vector<SearchHit> SearchOnPage(
      const std::string& search_string, int page, int context_length) override;

 private:
  // Default root outline item title.
  static const char* const DEFAULT_ROOT_OUTLINE_ITEM_TITLE;

  // Actual outline item implementation.
  class PDFOutlineItem : public OutlineItem {
   public:
    virtual ~PDFOutlineItem();
    // See Document::OutlineItem.
    int GetDestPage() const;
    // Factory method to create outline items from a fz_outline. This constructs
    // the entire outline hierarchy. Takes ownership of src.
    static PDFOutlineItem* Build(fz_context* ctx, fz_outline* src);

   private:
    // Destination page number.
    int _dest_page;
    // We disallow constructors; use the factory method Build() instead.
    explicit PDFOutlineItem(fz_outline* src);
    // Recursive construction, called by Build().
    static void BuildRecursive(
        fz_outline* src, std::vector<std::unique_ptr<OutlineItem>>* output);
  };

  // Cache for pdf_page.
  class PDFPageCache : public Cache<int, pdf_page*> {
   public:
    PDFPageCache(int cache_size, PDFDocument* parent);
    virtual ~PDFPageCache();
   protected:
    pdf_page* Load(const int& page) override;
    void Discard(const int& page, pdf_page* const& page_struct) override;
   private:
    // PDF document we belong to.
    PDFDocument* _parent;
    // Lock for Load() and Discard().
    std::mutex _mutex;
  };
  friend class PDFPageCache;

  // MuPDF structures.
  fz_context* _fz_context;
  pdf_document* _pdf_document;
  // Page cache.
  std::unique_ptr<PDFPageCache> _page_cache;
  // Lock guarding thread-unsafe parts of Render().
  std::mutex _render_mutex;

  // We disallow the constructor; use the factory method Open() instead.
  explicit PDFDocument(int page_cache_size);
  // We disallow copying because we store lots of heap allocated state.
  PDFDocument(const PDFDocument& other);
  PDFDocument& operator = (const PDFDocument& other);

  // Wrapper around pdf_load_page that implements caching. If _page_cache_size
  // is reached, throw out the oldest page. Will also attempt to load the pages
  // before and after specified page. Returns the loaded page.
  pdf_page* GetPage(int page);

  // Constructs a transformation matrix from the given parameters.
  fz_matrix Transform(float zoom, int rotation);
  // Returns a bounding box for the given page after applying transformations.
  fz_irect GetBoundingBox(pdf_page* page_struct, const fz_matrix& m);
};

#endif

