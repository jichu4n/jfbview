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

// This file provides a collection of utilities for working with fitz
// constructs.

#ifndef FITZ_UTILS_HPP
#define FITZ_UTILS_HPP

extern "C" {
#include "mupdf/fitz.h"
}

#include <string>

#include "document.hpp"

// Constructs a transformation matrix from the given parameters.
extern fz_matrix ComputeTransformMatrix(float zoom, int rotation);

// Returns a bounding box for the given page after applying transformations.
// NOT thread-safe.
extern fz_irect GetPageBoundingBox(
    fz_context* ctx, fz_page* page_struct, const fz_matrix& m);

// An outline item based on fitz's fz_outline.
class FitzOutlineItem : public Document::OutlineItem {
 public:
  virtual ~FitzOutlineItem();
  // See Document::OutlineItem.
  int GetDestPage() const;
  // Factory method to create outline items from a fz_outline. This constructs
  // the entire outline hierarchy. Does NOT take ownership. NOT thread-safe.
  static FitzOutlineItem* Build(fz_context* ctx, fz_outline* src);

 private:
  // Destination page number.
  int _dest_page;
  // We disallow constructors; use the factory method Build() instead.
  explicit FitzOutlineItem(fz_outline* src);
  // Recursive construction, called by Build().
  static void BuildRecursive(
      fz_outline* src, std::vector<std::unique_ptr<OutlineItem>>* output);
};

// Returns the text content of a page, using line_sep to separate lines. NOT
// thread-safe.
extern std::string GetPageText(
    fz_context* ctx, fz_page* page_struct, int line_sep = '\n');

// Generic smart pointer for Fitz resources.
template <typename FzT, void (*fz_drop_fn)(fz_context*, FzT*)>
class FitzScopedPtr {
 public:
  FitzScopedPtr(fz_context* ctx, FzT* raw_ptr) : _ctx(ctx), _raw_ptr(raw_ptr) {}
  ~FitzScopedPtr() { reset(); }

  FzT* get() const { return _raw_ptr; };
  FzT& operator*() const { return *_raw_ptr; }
  FzT* operator->() const { return _raw_ptr; }

  void reset(FzT* new_raw_ptr = nullptr) {
    if (_raw_ptr != nullptr) {
      (*fz_drop_fn)(_ctx, _raw_ptr);
    }
    _raw_ptr = new_raw_ptr;
  }

 private:
  fz_context* const _ctx;
  FzT* _raw_ptr;

  FitzScopedPtr(const FitzScopedPtr<FzT, fz_drop_fn>& other);
  FitzScopedPtr<FzT, fz_drop_fn>& operator=(
      const FitzScopedPtr<FzT, fz_drop_fn>& other);
};

// Smart pointer for fz_document.
typedef FitzScopedPtr<fz_document, &fz_drop_document> FitzDocumentScopedPtr;
// Smart pointer for fz_page.
typedef FitzScopedPtr<fz_page, &fz_drop_page> FitzPageScopedPtr;
// Smart pointer for fz_outline.
typedef FitzScopedPtr<fz_outline, &fz_drop_outline> FitzOutlineScopedPtr;
// Smart pointer for fz_device.
typedef FitzScopedPtr<fz_device, &fz_drop_device> FitzDeviceScopedPtr;
// Smart pointer for fz_pixmap.
typedef FitzScopedPtr<fz_pixmap, &fz_drop_pixmap> FitzPixmapScopedPtr;
// Smart pointer for fz_stext_page.
typedef FitzScopedPtr<fz_stext_page, &fz_drop_stext_page>
    FitzStextPageScopedPtr;

#endif

