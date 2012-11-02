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

// This file declares PDFDocument, an implementation of the Document abstraction
// using MuPDF.

#ifndef PDF_DOCUMENT_HPP
#define PDF_DOCUMENT_HPP

#include "document.hpp"
extern "C" {
#include <mupdf.h>
}
#include <queue>
#include <map>
#include <vector>

// Document implementation using MuPDF.
class PDFDocument: public Document {
 public:
  // Default size of page cache.
  enum { DEFAULT_PAGE_CACHE_SIZE = 5 };

  virtual ~PDFDocument();
  // Factory method to construct an instance of PDFDocument. path gives the path
  // to a PDF file. page_cache_size specifies the maximum number of pages to
  // store in memory.
  static PDFDocument *Open(const std::string &path,
                           int page_cache_size = DEFAULT_PAGE_CACHE_SIZE);
  // See Document.
  virtual int GetPageCount();
  // See Document.
  virtual const PageSize GetPageSize(int page, float zoom, int rotation);
  // See Document.
  virtual void Render(PixelWriter *pw, int page, float zoom, int rotation);
  // See Document.
  virtual const OutlineItem *GetOutline();
  // See Document.
  virtual int Lookup(const OutlineItem *item);
 private:
  // We disallow the constructor; use the factory method Open() instead.
  PDFDocument() {}
  // We disallow copying because we store lots of heap allocated state.
  PDFDocument(const PDFDocument &other);

  // Actual outline item implementation.
  class PDFOutlineItem: public OutlineItem {
   public:
    PDFOutlineItem(fz_outline *src);
    virtual ~PDFOutlineItem();
    int GetPageNum();
   private:
    fz_outline *_src;
  };
 private:
  // MuPDF structures.
  fz_context *_fz_context;
  pdf_document *_pdf_document;
  // Max page cache size.
  int _page_cache_size;
  // Page cache. Maps page number to page structure. Does not maintain
  // ownership.
  std::map<int, pdf_page *> _page_cache_map_num;
  // Page cache. Maps page structure to page number. Does not maintain
  // ownership.
  std::map<pdf_page *, int> _page_cache_map_struct;
  // Page cache, ordered by load age. Maintains ownership.
  std::queue<pdf_page *> _page_cache_queue;

  // Wrapper around pdf_load_page that implements caching. If _page_cache_size
  // is reached, throw out the oldest page. Will also attempt to load the pages
  // before and after specified page. Returns the loaded page.
  pdf_page *GetPage(int page);
  // Constructs a transformation matrix from the given parameters.
  fz_matrix Transform(float zoom, int rotation);
  // Returns a bounding box for the given page after applying transformations.
  fz_bbox GetBoundingBox(pdf_page *page_struct, const fz_matrix &m);
};

#endif

