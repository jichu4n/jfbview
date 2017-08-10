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

// This file defines PDFDocument, an implementation of the Document abstraction
// using MuPDF.

#include <stdint.h>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
extern "C" {
#include "mupdf/pdf.h"
}
#include "pdf_document.hpp"
#include "multithreading.hpp"
#include "string_utils.hpp"

#if MUPDF_VERSION < 10009
#  define pdf_drop_document pdf_close_document
#  define fz_stext_sheet fz_text_sheet
#  define fz_stext_page fz_text_page
#  define fz_stext_block fz_text_block
#  define fz_stext_line fz_text_line
#  define fz_stext_span fz_text_span
#  define fz_new_stext_device fz_new_text_device
#  define fz_new_stext_sheet fz_new_text_sheet
#  define fz_drop_stext_sheet fz_drop_text_sheet
#  define fz_new_stext_page_from_page fz_new_text_page_from_page
#  define fz_drop_stext_page fz_drop_text_page
#endif

#if MUPDF_VERSION >= 10007
#  define pdf_run_page(context, pdf_document, page, dev, matrix, cookie) \
       pdf_run_page((context), (page), (dev), (matrix), (cookie))
#  define pdf_bound_page(context, pdf_document, page, bbox) \
       pdf_bound_page((context), (page), (bbox))
#  if MUPDF_VERSION >= 10011
#    define pdf_drop_page(context, pdf_document, page) \
         fz_drop_page((context), &(page)->super)
#  else
#    define pdf_drop_page(context, pdf_document, page) \
         pdf_drop_page((context), (page))
#  endif
#else
#  define pdf_run_page(context, pdf_document, page, dev, matrix, cookie) \
       pdf_run_page((pdf_document), (page), (dev), (matrix), (cookie))
#  define pdf_bound_page(context, pdf_document, page, bbox) \
       pdf_bound_page((pdf_document), (page), (bbox))
#  define fz_drop_context fz_free_context
#  define fz_drop_device(context, dev) fz_free_device(dev)
#  define fz_drop_text_page fz_free_text_page
#  define fz_drop_text_sheet fz_free_text_sheet
#  define fz_drop_outline fz_free_outline
#  define pdf_drop_page(context, pdf_document, page) \
       pdf_free_page((pdf_document), (page))
#  define fz_begin_page(context, dev, rect, identity) \
       (fz_begin_page((dev), (rect), (identity)))
#  define fz_end_page(context, dev) \
       (fz_end_page(dev))
#  define pdf_count_pages(context, pdf_document) \
       (pdf_count_pages(pdf_document))
#  define pdf_load_page(context, pdf_document, page) \
       (pdf_load_page((pdf_document), (page)))
#  define pdf_load_outline(context, pdf_document) \
       (pdf_load_outline(pdf_document))
#  define pdf_close_document(context, pdf_document) \
       (pdf_close_document(pdf_document))
#endif

#if MUPDF_VERSION < 10010
#  define fz_new_pixmap_with_bbox(context, colorspace, bbox, alpha) \
       (fz_new_pixmap_with_bbox(context, colorspace, bbox))
#  define fz_new_draw_device(context, transform, dest) \
       (fz_new_draw_device(context, dest))
#endif

const char* const PDFDocument::DEFAULT_ROOT_OUTLINE_ITEM_TITLE =
    "TABLE OF CONTENTS";

PDFDocument* PDFDocument::Open(const std::string& path,
                               int page_cache_size) {
  fz_context* context = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
  pdf_document* raw_pdf_document = nullptr;
  fz_try(context) {
    raw_pdf_document = pdf_open_document(context, path.c_str());
    if ((raw_pdf_document == nullptr) ||
        (!pdf_count_pages(context, raw_pdf_document))) {
      fz_throw(
          context,
          FZ_ERROR_GENERIC,
          const_cast<char*>("Cannot open document \"%s\""),
          path.c_str());
    }
  } fz_catch(context) {
    if (raw_pdf_document != nullptr) {
      pdf_drop_document(context, raw_pdf_document);
    }
    fz_drop_context(context);
    return nullptr;
  }

  PDFDocument* document = new PDFDocument(page_cache_size);
  document->_fz_context = context;
  document->_pdf_document = raw_pdf_document;
  return document;
}

PDFDocument::PDFDocument(int page_cache_size)
    : _page_cache(new PDFPageCache(page_cache_size, this)) {
}

PDFDocument::~PDFDocument() {
  // Must destroy page cache explicitly first, since destroying cached pages
  // involves clearing MuPDF state, which requires document structures
  // (_pdf_document, _fz_context) to still exist.
  _page_cache.reset();

  pdf_drop_document(_fz_context, _pdf_document);
  fz_drop_context(_fz_context);
}

int PDFDocument::GetNumPages() {
  return pdf_count_pages(_fz_context, _pdf_document);
}

const Document::PageSize PDFDocument::GetPageSize(
    int page, float zoom, int rotation) {
  assert((page >= 0) && (page < GetNumPages()));
  const fz_irect& bbox = GetBoundingBox(
      GetPage(page), Transform(zoom, rotation));
  return PageSize(bbox.x1 - bbox.x0, bbox.y1 - bbox.y0);
}

void PDFDocument::Render(
    Document::PixelWriter* pw, int page, float zoom, int rotation) {
  assert((page >= 0) && (page < GetNumPages()));

  std::unique_lock<std::mutex> lock(_render_mutex);

  // 1. Init MuPDF structures.
  const fz_matrix& m = Transform(zoom, rotation);
  pdf_page* page_struct = GetPage(page);
  const fz_irect& bbox = GetBoundingBox(page_struct, m);
  fz_pixmap* pixmap = fz_new_pixmap_with_bbox(
      _fz_context, fz_device_rgb(_fz_context), &bbox, 1);
  fz_device* dev = fz_new_draw_device(_fz_context, &fz_identity, pixmap);

  // 2. Render page.
  fz_clear_pixmap_with_value(_fz_context, pixmap, 0xff);
  pdf_run_page(_fz_context, _pdf_document, page_struct, dev, &m, nullptr);

  // 3. Write pixmap to buffer. The page is vertically divided into n equal
  // stripes, each copied to pw by one thread.
  assert(fz_pixmap_components(_fz_context, pixmap) == 4);
  uint8_t* buffer = reinterpret_cast<uint8_t*>(
      fz_pixmap_samples(_fz_context, pixmap));
  const int num_cols = fz_pixmap_width(_fz_context, pixmap);
  const int num_rows = fz_pixmap_height(_fz_context, pixmap);
  ExecuteInParallel([=](int num_threads, int i) {
    const int num_rows_per_thread = num_rows / num_threads;
    const int y_begin = i * num_rows_per_thread;
    const int y_end = (i == num_threads - 1) ?
                          num_rows :
                          (i + 1) * num_rows_per_thread;
    uint8_t* p = buffer + y_begin * num_cols * 4;
    for (int y = y_begin; y < y_end; ++y) {
      for (int x = 0; x < num_cols; ++x) {
        pw->Write(x, y, p[0], p[1], p[2]);
        p += 4;
      }
    }
  });

  // 4. Clean up.
  fz_drop_device(_fz_context, dev);
  fz_drop_pixmap(_fz_context, pixmap);
}

const Document::OutlineItem* PDFDocument::GetOutline() {
  fz_outline* src = pdf_load_outline(_fz_context, _pdf_document);
  return (src == nullptr) ? nullptr : PDFOutlineItem::Build(_fz_context, src);
}

int PDFDocument::Lookup(const OutlineItem* item) {
  return (dynamic_cast<const PDFOutlineItem*>(item))->GetDestPage();
}

std::vector<Document::SearchHit> PDFDocument::SearchOnPage(
    const std::string& search_string, int page, int context_length) {
  const size_t margin =
      context_length > static_cast<int>(search_string.length()) ?
      (context_length - search_string.length() + 1) / 2 :
      0;

  std::vector<SearchHit> search_hits;
  const std::string& page_text = GetPageText(page, ' ');
  for (size_t pos = 0; ; ++pos) {
    if ((pos = CaseInsensitiveSearch(page_text, search_string, pos)) ==
        std::string::npos) {
      break;
    }
    const size_t context_start_pos = pos >= margin ?  pos - margin : 0;
    search_hits.emplace_back(
        page,
        page_text.substr(context_start_pos, context_length),
        pos - context_start_pos);
  }
  return search_hits;
}

std::string PDFDocument::GetPageText(int page, int line_sep) {
  // 1. Init MuPDF structures.
  pdf_page* page_struct = GetPage(page);
  fz_stext_sheet* text_sheet = fz_new_stext_sheet(_fz_context);

  // 2. Render page.
#if MUPDF_VERSION >= 10010
  fz_stext_options stext_options = { 0 };
  // See #elif MUPDF_VERSION >= 10009 block below.
  fz_stext_page* text_page = fz_new_stext_page_from_page(
      _fz_context, &(page_struct->super), text_sheet, &stext_options);
#elif MUPDF_VERSION >= 10009
  // The function below is a wrapper around fz_run_page that uses a fresh
  // device. We can't use pdf_run_page to gather the text for us.
  // These notes are also left in here in case MuPDF's API changes again.
  fz_stext_page* text_page = fz_new_stext_page_from_page(
      _fz_context, &(page_struct->super), text_sheet);
#else
  fz_stext_page* text_page = fz_new_text_page(_fz_context);
  fz_device* dev = fz_new_stext_device(_fz_context, text_sheet, text_page);
  // I've no idea what fz_{begin,end}_page do, but without them pdf_run_page
  // segfaults :-/
  fz_begin_page(_fz_context, dev, &fz_infinite_rect, &fz_identity);
  pdf_run_page(
      _fz_context, _pdf_document, page_struct, dev, &fz_identity, nullptr);
  fz_end_page(_fz_context, dev);
#endif

  // 3. Build text.
  std::string r;
  for (fz_page_block* page_block = text_page->blocks;
       page_block < text_page->blocks + text_page->len;
       ++page_block) {
    assert(page_block != nullptr);
    if (page_block->type != FZ_PAGE_BLOCK_TEXT) {
      continue;
    }
    fz_stext_block* const text_block = page_block->u.text;
    assert(text_block != nullptr);
    for (fz_stext_line* text_line = text_block->lines;
         text_line < text_block->lines + text_block->len;
         ++text_line) {
      assert(text_line != nullptr);
      for (fz_stext_span* text_span = text_line->first_span;
           text_span != nullptr;
           text_span = text_span->next) {
        for (int i = 0; i < text_span->len; ++i) {
          const int c = text_span->text[i].c;
          // A single UTF-8 character cannot take more than 4 bytes, but let's
          // go for 8.
          char buffer[8];
          const int num_bytes = fz_runetochar(buffer, c);
          assert(num_bytes <= static_cast<int>(sizeof(buffer)));
          buffer[num_bytes] = '\0';
          r += buffer;
        }
      }
      if (!isspace(r.back())) {
        r += line_sep;
      }
    }
  }

  // 4. Clean up.
  fz_drop_stext_page(_fz_context, text_page);
  fz_drop_stext_sheet(_fz_context, text_sheet);

  return r;
}

PDFDocument::PDFOutlineItem::~PDFOutlineItem() {
}

PDFDocument::PDFOutlineItem::PDFOutlineItem(fz_outline* src) {
  if (src == nullptr) {
    _dest_page = -1;
  } else {
    _title = src->title;
#if MUPDF_VERSION >= 10010
    _dest_page = src->page;
#else
    _dest_page = src->dest.ld.gotor.page;
#endif
  }
}

int PDFDocument::PDFOutlineItem::GetDestPage() const {
  return _dest_page;
}

PDFDocument::PDFOutlineItem* PDFDocument::PDFOutlineItem::Build(
    fz_context* ctx, fz_outline* src) {
  PDFOutlineItem* root = nullptr;
  std::vector<std::unique_ptr<OutlineItem>> items;
  BuildRecursive(src, &items);
  fz_drop_outline(ctx, src);
  if (items.empty()) {
    return nullptr;
  } else if (items.size() == 1) {
    root =  dynamic_cast<PDFOutlineItem*>(items[0].release());
  } else {
    root = new PDFOutlineItem(nullptr);
    root->_title = DEFAULT_ROOT_OUTLINE_ITEM_TITLE;
    root->_children.swap(items);
  }
  return root;
}

void PDFDocument::PDFOutlineItem::BuildRecursive(
    fz_outline* src,
    std::vector<std::unique_ptr<Document::OutlineItem>>* output) {
  assert(output != nullptr);
  for (fz_outline* i = src; i != nullptr; i = i->next) {
    PDFOutlineItem* item = new PDFOutlineItem(i);
    if (i->down != nullptr) {
      BuildRecursive(i->down, &(item->_children));
    }
    output->push_back(std::unique_ptr<Document::OutlineItem>(item));
  }
}

PDFDocument::PDFPageCache::PDFPageCache(int cache_size, PDFDocument* parent)
    : Cache<int, pdf_page*>(cache_size), _parent(parent) {
}

PDFDocument::PDFPageCache::~PDFPageCache() {
  Clear();
}

pdf_page* PDFDocument::PDFPageCache::Load(const int& page) {
  std::unique_lock<std::mutex> lock(_mutex);
  return pdf_load_page(_parent->_fz_context, _parent->_pdf_document, page);
}

void PDFDocument::PDFPageCache::Discard(
    const int& page, pdf_page* const& page_struct) {
  std::unique_lock<std::mutex> lock(_mutex);
  pdf_drop_page(_parent->_fz_context, _parent->_pdf_document, page_struct);
}

pdf_page* PDFDocument::GetPage(int page) {
  assert((page >= 0) && (page < GetNumPages()));
  return _page_cache->Get(page);
}

fz_matrix PDFDocument::Transform(float zoom, int rotation) {
  fz_matrix transformation_matrix, scale_matrix, rotate_matrix;
  fz_scale(&scale_matrix, zoom, zoom);
  fz_rotate(&rotate_matrix, rotation);
  fz_concat(&transformation_matrix, &scale_matrix, & rotate_matrix);
  return transformation_matrix;
}

fz_irect PDFDocument::GetBoundingBox(
    pdf_page* page_struct, const fz_matrix& m) {
  assert(page_struct != nullptr);
  fz_rect bbox;
  fz_irect ibbox;
  return *fz_round_rect(&ibbox, fz_transform_rect(
      pdf_bound_page(_fz_context, _pdf_document, page_struct, &bbox), &m));
}
