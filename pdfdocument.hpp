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

#ifndef PDFDOCUMENT_HPP
#define PDFDOCUMENT_HPP

#include "document.hpp"
extern "C" {
#include <mupdf.h>
}
#include <list>
#include <map>

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
  virtual const PageSize GetPageSize(int page);
  // See Document.
  virtual void *Render(int depth, int page, float zoom, int rotation);
  // See Document.
  virtual const OutlineItem *GetOutline();
  // See Document.
  virtual int Lookup(const OutlineItem &item);
 private:
  // We disallow the constructor; use the factory method Open() instead.
  PDFDocument();
  // We disallow copying because we store lots of heap allocated state.
  PDFDocument(const PDFDocument &other);

  // Actual outline item implementation.
  class PDFOutlineItem: public OutlineItem {
   public:
    PDFOutlineItem(fz_outline *src);
    virtual ~PDFOutlineItem();
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
  std::map<int, pdf_page *> _page_cache_map;
  // Page cache, ordered by load age. Maintains ownership.
  std::list<pdf_page *> _page_cache_list;

  // Wrapper around pdf_load_page that implements caching. If _page_cache_size
  // is reached, throw out the oldest page. Will also attempt to load the pages
  // before and after specified page. Returns the loaded page.
  pdf_page *GetPage(int page);
};

#endif

