/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2020-2020 Chuan Ji                                         *
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

// This file defines FitzDocument, an implementation of the Document
// abstraction using Fitz.

#include "fitz_document.hpp"

#include <cassert>

#include "fitz_utils.hpp"
#include "multithreading.hpp"

namespace {

fz_irect GetPageBoundingBox(
    fz_context* ctx, std::mutex* fz_mutex, fz_page* page_struct,
    const fz_matrix& m) {
  assert(page_struct != nullptr);
  std::lock_guard<std::mutex> lock(*fz_mutex);
  return fz_round_rect(fz_transform_rect(fz_bound_page(ctx, page_struct), m));
}

class FitzDocumentPageCache : public Cache<int, fz_page*> {
 public:
  FitzDocumentPageCache(
      fz_context* fz_ctx, fz_document* fz_doc, std::mutex* fz_mutex,
      int page_cache_size)
      : Cache<int, fz_page*>(page_cache_size),
        _fz_ctx(fz_ctx),
        _fz_doc(fz_doc),
        _fz_mutex(fz_mutex) {}
  virtual ~FitzDocumentPageCache() { Clear(); }

 protected:
  fz_page* Load(const int& page) override {
    std::lock_guard<std::mutex> lock(*_fz_mutex);
    return fz_load_page(_fz_ctx, _fz_doc, page);
  }

  void Discard(const int& page, fz_page* const& page_struct) override {
    std::lock_guard<std::mutex> lock(*_fz_mutex);
    fz_drop_page(_fz_ctx, page_struct);
  }

 private:
  // Pointer to parent FitzDocument's MuPDF structures. Does NOT have ownerhip.
  fz_context* _fz_ctx;
  fz_document* _fz_doc;
  // Pointer to parent FitzDocument's mutex guarding MuPDF structures. Does NOT
  // have ownership.
  std::mutex* _fz_mutex;
};

}  // namespace

Document* FitzDocument::Open(
    const std::string& path, const std::string* password, int page_cache_size) {
  fz_context* fz_ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
  fz_register_document_handlers(fz_ctx);
  // Disable warning messages in the console.
  fz_set_warning_callback(
      fz_ctx, [](void* user, const char* message) {}, nullptr);

  fz_document* fz_doc = nullptr;
  fz_try(fz_ctx) {
    fz_doc = fz_open_document(fz_ctx, path.c_str());
    if ((fz_doc == nullptr) || (!fz_count_pages(fz_ctx, fz_doc))) {
      fz_throw(
          fz_ctx, FZ_ERROR_GENERIC,
          const_cast<char*>("Cannot open document \"%s\""), path.c_str());
    }
    if (fz_needs_password(fz_ctx, fz_doc)) {
      if (password == nullptr) {
        fz_throw(
            fz_ctx, FZ_ERROR_GENERIC,
            const_cast<char*>(
                "Document \"%s\" is password protected.\n"
                "Please provide the password with \"-P <password>\"."),
            path.c_str());
      }
      if (!fz_authenticate_password(fz_ctx, fz_doc, password->c_str())) {
        fz_throw(
            fz_ctx, FZ_ERROR_GENERIC,
            const_cast<char*>("Incorrect password for document \"%s\"."),
            path.c_str());
      }
    }
  }
  fz_catch(fz_ctx) {
    if (fz_doc != nullptr) {
      fz_drop_document(fz_ctx, fz_doc);
    }
    fz_drop_context(fz_ctx);
    return nullptr;
  }

  return new FitzDocument(fz_ctx, fz_doc, page_cache_size);
}

FitzDocument::FitzDocument(
    fz_context* fz_ctx, fz_document* fz_doc, int page_cache_size)
    : _fz_ctx(fz_ctx),
      _fz_doc(fz_doc),
      _page_cache(new FitzDocumentPageCache(
          fz_ctx, fz_doc, &_fz_mutex, page_cache_size)) {
  assert(_fz_ctx != nullptr);
  assert(_fz_doc != nullptr);
}

FitzDocument::~FitzDocument() {
  // Must destroy page cache explicitly first, since destroying cached pages
  // involves clearing MuPDF state, which requires document structures
  // (_fz_doc, _fz_ctx) to still exist.
  _page_cache.reset();

  std::lock_guard<std::mutex> lock(_fz_mutex);
  fz_drop_document(_fz_ctx, _fz_doc);
  fz_drop_context(_fz_ctx);
}

int FitzDocument::GetNumPages() {
  std::lock_guard<std::mutex> lock(_fz_mutex);
  return fz_count_pages(_fz_ctx, _fz_doc);
}

const Document::PageSize FitzDocument::GetPageSize(
    int page, float zoom, int rotation) {
  assert((page >= 0) && (page < GetNumPages()));
  const fz_irect& bbox = GetPageBoundingBox(
      _fz_ctx, &_fz_mutex, GetPage(page),
      ComputeTransformMatrix(zoom, rotation));
  return PageSize(bbox.x1 - bbox.x0, bbox.y1 - bbox.y0);
}

void FitzDocument::Render(
    Document::PixelWriter* pw, int page, float zoom, int rotation) {
  assert((page >= 0) && (page < GetNumPages()));

  // 1. Init MuPDF structures.
  const fz_matrix& m = ComputeTransformMatrix(zoom, rotation);
  fz_page* page_struct = GetPage(page);
  const fz_irect& bbox =
      GetPageBoundingBox(_fz_ctx, &_fz_mutex, page_struct, m);
  std::lock_guard<std::mutex> lock(_fz_mutex);
  fz_pixmap* pixmap = fz_new_pixmap_with_bbox(
      _fz_ctx, fz_device_rgb(_fz_ctx), bbox, nullptr, 1);
  fz_device* dev = fz_new_draw_device(_fz_ctx, fz_identity, pixmap);

  // 2. Render page.
  fz_clear_pixmap_with_value(_fz_ctx, pixmap, 0xff);
  fz_run_page(_fz_ctx, page_struct, dev, m, nullptr);

  // 3. Write pixmap to buffer. The page is vertically divided into n equal
  // stripes, each copied to pw by one thread.
  assert(fz_pixmap_components(_fz_ctx, pixmap) == 4);
  uint8_t* buffer =
      reinterpret_cast<uint8_t*>(fz_pixmap_samples(_fz_ctx, pixmap));
  const int num_cols = fz_pixmap_width(_fz_ctx, pixmap);
  const int num_rows = fz_pixmap_height(_fz_ctx, pixmap);
  ExecuteInParallel([=](int num_threads, int i) {
    const int num_rows_per_thread = num_rows / num_threads;
    const int y_begin = i * num_rows_per_thread;
    const int y_end =
        (i == num_threads - 1) ? num_rows : (i + 1) * num_rows_per_thread;
    uint8_t* p = buffer + y_begin * num_cols * 4;
    for (int y = y_begin; y < y_end; ++y) {
      for (int x = 0; x < num_cols; ++x) {
        pw->Write(x, y, p[0], p[1], p[2]);
        p += 4;
      }
    }
  });

  // 4. Clean up.
  fz_close_device(_fz_ctx, dev);
  fz_drop_device(_fz_ctx, dev);
  fz_drop_pixmap(_fz_ctx, pixmap);
}

fz_page* FitzDocument::GetPage(int page) {
  assert((page >= 0) && (page < GetNumPages()));
  return _page_cache->Get(page);
}

