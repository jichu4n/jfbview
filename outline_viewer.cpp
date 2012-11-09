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

// This file defines the document outline viewer.

#include "outline_viewer.hpp"
#include <stack>

OutlineViewer::OutlineViewer(const Document::OutlineItem *outline)
    : _outline(outline) {
}

bool OutlineViewer::Show(int *dest_page) {
  if (_outline == NULL) {
    return false;
  }
}

void OutlineViewer::Flatten() {
  _lines.clear();
  FlattenRecursive(_outline, 0);
}

void OutlineViewer::FlattenRecursive(const Document::OutlineItem *item,
                                     int depth) {
  if (item == NULL) {
    return;
  }

  _lines.push_back(std::string());
  int line = _lines.size() - 1;

  for (int i = 0; i < depth; ++i) {
    _lines[line] += "| ";
  }
  if (item->GetChildrenCount()) {
    if (_expanded_items.count(item)) {
      _lines[line] += '+';
      for (int i = 0; i < item->GetChildrenCount(); ++i) {
        FlattenRecursive(item->GetChild(i), depth + 1);
      }
    } else {
      _lines[line] += '-';
    }
  } else {
      _lines[line] += ' ';
  }
  _lines[line] += item->GetTitle();
}

