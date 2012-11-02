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

// This file defines PDFDocument, an implementation of the Document abstraction
// using MuPDF.

#include "pdfdocument.hpp"

PDFDocument *PDFDocument::Open(const std::string &path,
                               int page_cache_size) {
  fz_context *context = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
  pdf_document *raw_pdf_document = pdf_open_document(context, path.c_str());
  if (raw_pdf_document == NULL) {
    fz_free_context(context);
    return NULL;
  }

  PDFDocument *document = new PDFDocument();
  document->_fz_context = fz_context;
  document->_pdf_document = raw_pdf_document;
  document->_page_cache_size = page_cache_size;
  return document;
}

PDFDocument::~PDFDocument() {
  for (std::list<pdf_page *>::iterator i = _page_cache_list.begin();
       i != _page_cache_list.end(); ++i) {
    pdf_free_page(_pdf_document, *i);
  }
  pdf_close_document(_pdf_document);
  fz_free_context(_fz_context);
}

int PDFDocument::GetPageCount() {
  return pdf_count_pages(_pdf_document);
}

pdf_page *PDFDocument::GetPage(int page) {
  
}

