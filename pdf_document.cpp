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

#include "pdf_document.hpp"
#include "multithreading.hpp"
#include <cassert>
#include <cstring>
#include <stdint.h>
#include <algorithm>
#include <vector>

Document *PDFDocument::Open(const std::string &path,
                               int page_cache_size) {
  fz_context *context = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
  pdf_document *raw_pdf_document = NULL;
  fz_try(context) {
    raw_pdf_document = pdf_open_document(context, path.c_str());
    if ((raw_pdf_document == NULL) || (!pdf_count_pages(raw_pdf_document))) {
      fz_throw(context, const_cast<char *>("Cannot open document \"%s\""),
               path.c_str());
    }
  } fz_catch(context) {
    if (raw_pdf_document != NULL) {
      pdf_close_document(raw_pdf_document);
    }
    fz_free_context(context);
    return NULL;
  }

  PDFDocument *document = new PDFDocument(page_cache_size);
  document->_fz_context = context;
  document->_pdf_document = raw_pdf_document;
  return document;
}

PDFDocument::PDFDocument(int page_cache_size)
    : _page_cache(new PDFPageCache(page_cache_size, this)) {
}

PDFDocument::~PDFDocument() {
  delete _page_cache;
  pdf_close_document(_pdf_document);
  fz_free_context(_fz_context);
}

int PDFDocument::GetPageCount() {
  return pdf_count_pages(_pdf_document);
}

const Document::PageSize PDFDocument::GetPageSize(
    int page, float zoom, int rotation) {
  assert((page >= 0) && (page < GetPageCount()));
  const fz_irect &bbox = GetBoundingBox(
      GetPage(page), Transform(zoom, rotation));
  return PageSize(bbox.x1 - bbox.x0, bbox.y1 - bbox.y0);
}

void PDFDocument::Render(Document::PixelWriter *pw, int page, float zoom,
                         int rotation) {
  assert((page >= 0) && (page < GetPageCount()));

  std::unique_lock<std::mutex> lock(_render_mutex);

  // 1. Init MuPDF structures.
  const fz_matrix &m = Transform(zoom, rotation);
  pdf_page *page_struct = GetPage(page);
  const fz_irect &bbox = GetBoundingBox(page_struct, m);
  fz_pixmap *pixmap = fz_new_pixmap_with_bbox(_fz_context, fz_device_rgb, &bbox);
  fz_device *dev = fz_new_draw_device(_fz_context, pixmap);

  // 2. Render page.
  fz_clear_pixmap_with_value(_fz_context, pixmap, 0xff);
  pdf_run_page(_pdf_document, page_struct, dev, &m, NULL);

  // 3. Write pixmap to buffer. The page is vertically divided into n equal
  // stripes, each copied to pw by one thread.
  assert(fz_pixmap_components(_fz_context, pixmap) == 4);
  uint8_t *buffer = reinterpret_cast<uint8_t *>(
      fz_pixmap_samples(_fz_context, pixmap));
  const int num_cols = fz_pixmap_width(_fz_context, pixmap);
  const int num_rows = fz_pixmap_height(_fz_context, pixmap);
  ExecuteInParallel([=](int num_threads, int i) {
    const int num_rows_per_thread = num_rows / num_threads;
    const int y_begin = i * num_rows_per_thread;
    const int y_end = (i == num_threads - 1) ?
                          num_rows :
                          (i + 1) * num_rows_per_thread;
    uint8_t *p = buffer + y_begin * num_cols * 4;
    for (int y = y_begin; y < y_end; ++y) {
      for (int x = 0; x < num_cols; ++x) {
        pw->Write(x, y, p[0], p[1], p[2]);
        p += 4;
      }
    }
  });

  // 4. Clean up.
  fz_free_device(dev);
  fz_drop_pixmap(_fz_context, pixmap);
}

const Document::OutlineItem *PDFDocument::GetOutline() {
  fz_outline *src = pdf_load_outline(_pdf_document);
  return (src == NULL) ? NULL : PDFOutlineItem::Build(_fz_context, src);
}

int PDFDocument::Lookup(const OutlineItem *item) {
  return (dynamic_cast<const PDFOutlineItem *>(item))->GetDestPage();
}

PDFDocument::PDFOutlineItem::~PDFOutlineItem() {
}

PDFDocument::PDFOutlineItem::PDFOutlineItem(fz_outline *src) {
  if (src == NULL) {
    _dest_page = -1;
  } else {
    _title = src->title;
    _dest_page = src->dest.ld.gotor.page;
  }
}

int PDFDocument::PDFOutlineItem::GetDestPage() const {
  return _dest_page;
}

PDFDocument::PDFOutlineItem *PDFDocument::PDFOutlineItem::Build(
    fz_context *ctx, fz_outline *src) {
  PDFOutlineItem *root = NULL;
  std::vector<OutlineItem *> items;
  BuildRecursive(src, &items);
  fz_free_outline(ctx, src);
  if (items.empty()) {
    return NULL;
  } else if (items.size() == 1) {
    root =  dynamic_cast<PDFOutlineItem *>(items[0]);
  } else {
    root = new PDFOutlineItem(NULL);
    root->_title = "TABLE OF CONTENTS";
    root->_children.insert(root->_children.begin(), items.begin(), items.end());
  }
  return root;
}

void PDFDocument::PDFOutlineItem::BuildRecursive(
    fz_outline *src,
    std::vector<Document::OutlineItem *> *output) {
  assert(output != NULL);
  for (fz_outline *i = src; i != NULL; i = i->next) {
    PDFOutlineItem *item = new PDFOutlineItem(i);
    if (i->down != NULL) {
      BuildRecursive(i->down, &(item->_children));
    }
    output->push_back(item);
  }
}

PDFDocument::PDFPageCache::PDFPageCache(int cache_size, PDFDocument *parent)
    : Cache<int, pdf_page *>(cache_size), _parent(parent) {
}

PDFDocument::PDFPageCache::~PDFPageCache() {
  Clear();
}

pdf_page *PDFDocument::PDFPageCache::Load(const int &page) {
  std::unique_lock<std::mutex> lock(_mutex);
  return pdf_load_page(_parent->_pdf_document, page);
}

void PDFDocument::PDFPageCache::Discard(const int &page,
                                        pdf_page * &page_struct) {
  std::unique_lock<std::mutex> lock(_mutex);
  pdf_free_page(_parent->_pdf_document, page_struct);
}

pdf_page *PDFDocument::GetPage(int page) {
  assert((page >= 0) && (page < GetPageCount()));
  return _page_cache->Get(page);
}

fz_matrix PDFDocument::Transform(float zoom, int rotation) {
  fz_matrix transformation_matrix, scale_matrix, rotate_matrix;
  fz_scale(&scale_matrix, zoom, zoom);
  fz_rotate(&rotate_matrix, rotation);
  fz_concat(&transformation_matrix, &scale_matrix, & rotate_matrix);
  return transformation_matrix;
}

fz_irect PDFDocument::GetBoundingBox(pdf_page *page_struct, const fz_matrix &m) {
  assert(page_struct != NULL);
  fz_rect bbox;
  fz_irect ibbox;
  return *fz_round_rect(&ibbox, fz_transform_rect(
      pdf_bound_page(_pdf_document, page_struct, &bbox), &m));
}
