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

struct fz_context;
struct fz_outline;
struct pdf_document;

// Document implementation using MuPDF.
class PDFDocument: public Document {
 public:
  virtual ~PDFDocument();
  // Factory method to construct an instance of PDFDocument.
  static PDFDocument *Open(const std::string &path);
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
  fz_context *_fz_context;
  pdf_document *_pdf_document;
};

#endif

