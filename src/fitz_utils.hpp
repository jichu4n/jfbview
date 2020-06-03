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

#endif

