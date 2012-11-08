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
#include <pthread.h>
#include <cassert>
#include <cstring>
#include <stdint.h>
#include <unistd.h>
#include <algorithm>
#include <new>

PDFDocument *PDFDocument::Open(const std::string &path,
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
  const fz_bbox &bbox = GetBoundingBox(
      GetPage(page), Transform(zoom, rotation));
  return PageSize(bbox.x1 - bbox.x0, bbox.y1 - bbox.y0);
}

namespace {

// Argument to RenderWorker.
struct RenderWorkerArgs {
  // The first and (last + 1) rows to render. The area rendered is bounded by
  // (0, YBegin) at the top-left to (Width, YEnd - 1) at the bottom-right.
  int YBegin, YEnd;
  // The number of columns; the area rendered is bounded by (0, YBegin) at the
  // top-left to (Width - 1, YEnd - 1) at the bottom-right.
  int Width;
  // The pixmap memory buffer. Each pixel takes up 3 bytes, one each for r, g,
  // and b components in that order. This should be the beginning of the entire
  // pixmap buffer.
  uint8_t *Buffer;
  // The pixel writer.
  Document::PixelWriter *Writer;
};

// Renders a vertical segment on a thread, passed to pthread_create. The
// argument should be a RenderWorkerArgs.
void *RenderWorker(void *_args) {
  RenderWorkerArgs *args = reinterpret_cast<RenderWorkerArgs *>(_args);
  uint8_t *p = args->Buffer + args->YBegin * args->Width * 4;
  for (int y = args->YBegin; y < args->YEnd; ++y) {
    for (int x = 0; x < args->Width; ++x) {
      args->Writer->Write(x, y, p[0], p[1], p[2]);
      p += 4;
    }
  }
  return NULL;
}

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

  // 3. Write pixmap to buffer using #CPUs threads.
  assert(fz_pixmap_components(_fz_context, pixmap) == 4);
  uint8_t *p = reinterpret_cast<uint8_t *>(
      fz_pixmap_samples(_fz_context, pixmap));
  int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
  int num_rows_per_thread = fz_pixmap_height(_fz_context, pixmap) / num_threads;
  pthread_t *threads = new pthread_t[num_threads];
  RenderWorkerArgs *args = new RenderWorkerArgs[num_threads];
  for (int i = 0; i < num_threads; ++i) {
    args[i].YBegin = i * num_rows_per_thread;
    args[i].YEnd = (i == num_threads - 1) ?
        fz_pixmap_height(_fz_context, pixmap) :
        (i + 1) * num_rows_per_thread;
    args[i].Width = fz_pixmap_width(_fz_context, pixmap);
    args[i].Buffer = p;
    args[i].Writer = pw;
    pthread_create(&(threads[i]), NULL, &RenderWorker, &(args[i]));
  }
  for (int i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }
  delete[] args;
  delete[] threads;

  // 4. Clean up.
  fz_free_device(dev);
  fz_drop_pixmap(_fz_context, pixmap);
}

const Document::OutlineItem *PDFDocument::GetOutline() {
  fz_outline *src = pdf_load_outline(_pdf_document);
  return (src == NULL) ? NULL : PDFOutlineItem::Build(this, src);
}

int PDFDocument::Lookup(const OutlineItem *item) {
  return (dynamic_cast<const PDFOutlineItem *>(item))->GetPageNum();
}

PDFDocument::PDFOutlineItem::~PDFOutlineItem() {
  if (_is_root) {
    fz_free_outline(_parent->_fz_context, _src);
  }
}

PDFDocument::PDFOutlineItem::PDFOutlineItem(PDFDocument *parent,
                                            fz_outline *src)
    : _parent(parent), _src(src), _is_root(false) {
  _title = (src == NULL) ? "" : src->title;
}

int PDFDocument::PDFOutlineItem::GetPageNum() const {
  return _src->dest.ld.gotor.page;
}

PDFDocument::PDFOutlineItem *PDFDocument::PDFOutlineItem::Build(
    PDFDocument *parent, fz_outline *src) {
  PDFOutlineItem *root = NULL;
  std::vector<OutlineItem *> items;
  BuildRecursive(parent, src, &items);
  if (items.empty()) {
    return NULL;
  } else if (items.size() == 1) {
    root =  dynamic_cast<PDFOutlineItem *>(items[0]);
  } else {
    root = new PDFOutlineItem(parent, NULL);
    root->_children.insert(root->_children.begin(), items.begin(), items.end());
  }
  root->_is_root = true;
  return root;
}

void PDFDocument::PDFOutlineItem::BuildRecursive(
    PDFDocument *parent, fz_outline *src,
    std::vector<Document::OutlineItem *> *output) {
  assert(output != NULL);
  for (fz_outline *i = src; i != NULL; i = i->next) {
    PDFOutlineItem *item = new PDFOutlineItem(parent, i);
    if (i->down != NULL) {
      BuildRecursive(parent, i->down, &(item->_children));
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
  return pdf_load_page(_parent->_pdf_document, page);
}

void PDFDocument::PDFPageCache::Discard(const int &page,
                                        pdf_page * &page_struct) {
  pdf_free_page(_parent->_pdf_document, page_struct);
}

pdf_page *PDFDocument::GetPage(int page) {
  assert((page >= 0) && (page < GetPageCount()));
  return _page_cache->Get(page);
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
