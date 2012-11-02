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
#include <cassert>
#include <cstring>
#include <algorithm>
#include <new>

PDFDocument *PDFDocument::Open(const std::string &path,
                               int page_cache_size) {
  fz_context *context = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
  pdf_document *raw_pdf_document = pdf_open_document(context, path.c_str());
  if (raw_pdf_document == NULL) {
    fz_free_context(context);
    return NULL;
  }

  PDFDocument *document = new PDFDocument();
  document->_fz_context = context;
  document->_pdf_document = raw_pdf_document;
  document->_page_cache_size = page_cache_size;
  return document;
}

PDFDocument::~PDFDocument() {
  while (_page_cache_queue.size()) {
    pdf_free_page(_pdf_document, _page_cache_queue.front());
    _page_cache_queue.pop();
  }
  pdf_close_document(_pdf_document);
  fz_free_context(_fz_context);
}

int PDFDocument::GetPageCount() {
  return pdf_count_pages(_pdf_document);
}

const Document::PageSize PDFDocument::GetPageSize(
    int page, float zoom, int rotation) {
  assert((page >= 0) && (page < GetPageCount()));
  const fz_bbox &bbox = GetBoundingBox(
      GetPage(page), Transform(zoom, rotation));
  return PageSize(bbox.x1 - bbox.x0, bbox.y1 - bbox.y0);
}

void PDFDocument::Render(Document::PixelWriter *pw, int page, float zoom,
                         int rotation) {
  assert((page >= 0) && (page < GetPageCount()));

  // 1. Init MuPDF structures.
  pdf_page *page_struct = GetPage(page);
  const fz_matrix &m = Transform(zoom, rotation);
  const fz_bbox &bbox = GetBoundingBox(page_struct, m);
  fz_pixmap *pixmap = fz_new_pixmap_with_bbox(_fz_context, fz_device_rgb, bbox);
  fz_device *dev = fz_new_draw_device(_fz_context, pixmap);

  // 2. Render page.
  fz_clear_pixmap_with_value(_fz_context, pixmap, 0xff);
  pdf_run_page(_pdf_document, page_struct, dev, m, NULL);

  // 3. Write pixmap to buffer.
  unsigned char *p = fz_pixmap_samples(_fz_context, pixmap);
  assert(fz_pixmap_components(_fz_context, pixmap) == 3);
  for (int y = 0; y < fz_pixmap_height(_fz_context, pixmap); ++y) {
    for (int x = 0; x < fz_pixmap_width(_fz_context, pixmap); ++x) {
      pw->Write(p[0], p[1], p[2], x, y);
      p += 3;
    }
  }

  // 4. Clean up.
  fz_free_device(dev);
  fz_drop_pixmap(_fz_context, pixmap);
}

const OutlineItem *PDFDocument::GetOutline() {
  fz_outline *src = pdf_load_outline(_pdf_document);
  return (src == NULL) ? NULL : new PDFOutlineItem(src);
}

int PDFDocument::Lookup(const OutlineItem *item) {
  return (dynamic_cast<PDFOutlineItem *>(item))->GetPageNum();
}

PDFDocument::PDFOutlineItem::PDFOutlineItem(fz_outline *src)
    : _src(src) {
  _title = src->title;
  // TODO: write this.
}

int PDFDocument::PDFOutlineItem::GetPageNum() {
  return _src->dest.ld.gotor.page;
}

pdf_page *PDFDocument::GetPage(int page) {
  assert((page >= 0) && (page < GetPageCount()));
  int required_cache_slots = 0;
  int page_start = std::max(0, page - 1),
      page_end = std::min(GetPageCount() - 1, page + 1);
  for (int i = page_start; i <= page_end; ++i) {
    if (!_page_cache_map_num.count(i)) {
      ++required_cache_slots;
    }
  }
  int pages_to_evict = (static_cast<int>(_page_cache_queue.size()) +
      required_cache_slots) - _page_cache_size;
  for (int i = 0; i < pages_to_evict; ++i) {
    pdf_page *victim = _page_cache_queue.front();
    int victim_num = _page_cache_map_struct.find(victim)->second;
    pdf_free_page(_pdf_document, victim);
    _page_cache_queue.pop();
    _page_cache_map_num.erase(victim_num);
    _page_cache_map_struct.erase(victim);
  }
  for (int i = page_start; i <= page_end; ++i) {
    if (!_page_cache_map_num.count(i)) {
      pdf_page *new_page = pdf_load_page(_pdf_document, i);
      assert(new_page != NULL);
      _page_cache_queue.push(new_page);
      _page_cache_map_num[i] = new_page;
      _page_cache_map_struct[new_page] = i;
    }
  }
  return _page_cache_map_num[page];
}

fz_matrix PDFDocument::Transform(float zoom, int rotation) {
  fz_matrix m = fz_identity;
  m = fz_concat(m, fz_scale(zoom, zoom));
  m = fz_concat(m, fz_rotate(-rotation));
  return m;
}

fz_bbox PDFDocument::GetBoundingBox(pdf_page *page_struct, const fz_matrix &m) {
  assert(page_struct != NULL);
  return fz_round_rect(fz_transform_rect(
      m, pdf_bound_page(_pdf_document, page_struct)));
}
