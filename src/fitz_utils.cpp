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

#include "fitz_utils.hpp"

#include <cassert>

fz_matrix ComputeTransformMatrix(float zoom, int rotation) {
  fz_matrix transformation_matrix, scale_matrix, rotate_matrix;
  scale_matrix = fz_scale(zoom, zoom);
  rotate_matrix = fz_rotate(rotation);
  transformation_matrix = fz_concat(scale_matrix, rotate_matrix);
  return transformation_matrix;
}

fz_irect GetPageBoundingBox(
    fz_context* ctx, fz_page* page_struct, const fz_matrix& m) {
  assert(page_struct != nullptr);
  return fz_round_rect(fz_transform_rect(fz_bound_page(ctx, page_struct), m));
}

namespace {

const char* const DEFAULT_ROOT_OUTLINE_ITEM_TITLE = "TABLE OF CONTENTS";

}  // namespace

FitzOutlineItem::~FitzOutlineItem() {}

FitzOutlineItem::FitzOutlineItem(fz_outline* src) {
  if (src == nullptr) {
    _dest_page = -1;
  } else {
    _title = src->title;
    _dest_page = src->page;
  }
}

int FitzOutlineItem::GetDestPage() const { return _dest_page; }

FitzOutlineItem* FitzOutlineItem::Build(fz_context* ctx, fz_outline* src) {
  FitzOutlineItem* root = nullptr;
  std::vector<std::unique_ptr<OutlineItem>> items;
  BuildRecursive(src, &items);
  if (items.empty()) {
    return nullptr;
  } else if (items.size() == 1) {
    root = dynamic_cast<FitzOutlineItem*>(items[0].release());
  } else {
    root = new FitzOutlineItem(nullptr);
    root->_title = DEFAULT_ROOT_OUTLINE_ITEM_TITLE;
    root->_children.swap(items);
  }
  return root;
}

void FitzOutlineItem::BuildRecursive(
    fz_outline* src,
    std::vector<std::unique_ptr<Document::OutlineItem>>* output) {
  assert(output != nullptr);
  for (fz_outline* i = src; i != nullptr; i = i->next) {
    FitzOutlineItem* item = new FitzOutlineItem(i);
    if (i->down != nullptr) {
      BuildRecursive(i->down, &(item->_children));
    }
    output->push_back(std::unique_ptr<Document::OutlineItem>(item));
  }
}

std::string GetPageText(fz_context* ctx, fz_page* page_struct, int line_sep) {
  // 1. Render page.
  fz_stext_options stext_options = {0};
  FitzStextPageScopedPtr text_page(
      ctx, fz_new_stext_page_from_page(ctx, page_struct, &stext_options));

  // 2. Build text.
  std::string r;
  for (fz_stext_block* text_block = text_page->first_block;
       text_block != nullptr; text_block = text_block->next) {
    if (text_block->type != FZ_STEXT_BLOCK_TEXT) {
      continue;
    }
    for (fz_stext_line* text_line = text_block->u.t.first_line;
         text_line != nullptr; text_line = text_line->next) {
      for (fz_stext_char* text_char = text_line->first_char;
           text_char != nullptr; text_char = text_char->next) {
        {
          const int c = text_char->c;
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

  return r;
}

