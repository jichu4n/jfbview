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

// This file declares the document outline viewer.

#ifndef OUTLINE_VIEWER_HPP
#define OUTLINE_VIEWER_HPP

#include <curses.h>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include "document.hpp"
#include "ui_view.hpp"

// Outline viewer class. This class stores the expansion and focus states
// between invocations.
class OutlineViewer : public UIView {
 public:
  // Constructs an instance of OutlineViewer that displays the given Outline.
  // Takes ownership of the outline object.
  explicit OutlineViewer(const Document::OutlineItem* outline);
  virtual ~OutlineViewer();
  // Displays the outline view and enter the event loop. If the user selected a
  // page to jump to, returns the selected outline item. Otherwise returns
  // nullptr.
  const Document::OutlineItem* Run();

 protected:
  void Render() override;
  void ProcessKey(int key) override;

 private:
  // An outline item with display related annotation.
  struct Line {
    // The outline item this display item represents.
    const Document::OutlineItem* OutlineItem;
    // The displayed string.
    std::string Label;
  };

  // The current key processing mode.
  enum {
    REGULAR,   // Regular mode.
    FOLD,      // Processing the next character after a 'z'.
  } _key_processing_mode;

  // Handle to the outline we display.
  std::unique_ptr<const Document::OutlineItem> _outline;
  // The set of expanded (unfolded) items.
  std::set<const Document::OutlineItem *> _expanded_items;
  // The set of all expandedable items (i.e., those with children). Built by
  // Flatten().
  std::set<const Document::OutlineItem *> _all_expandable_items;
  // The outline flattened out into an array of lines.
  std::vector<Line> _lines;
  // Index in _lines of the currently highlighted item.
  int _selected_index;
  // Index in _lines of the first displayed item.
  int _first_index;
  // The selected outline item.
  const Document::OutlineItem* _selected_item;

  // Flatten out _outline to _lines, according to _expanded_items.
  void Flatten();
  // Recursively flatten the outline item using preorder traversal.
  void FlattenRecursive(const Document::OutlineItem* item, int depth);
};

#endif

