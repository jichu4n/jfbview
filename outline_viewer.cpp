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
#include <cassert>
#include <algorithm>

OutlineViewer::OutlineViewer(const Document::OutlineItem* outline)
    : _outline(outline), _selected_index(0),
      _first_index(0) {
  if (_outline != nullptr) {
    _expanded_items.insert(_outline.get());
    Flatten();
  }
}

OutlineViewer::~OutlineViewer() {}

const Document::OutlineItem* OutlineViewer::Run() {
  if (_outline == nullptr) {
    return nullptr;
  }

  _selected_item = nullptr;
  _key_processing_mode = REGULAR;

  EventLoop();

  return _selected_item;
}



void OutlineViewer::Flatten() {
  _lines.clear();
  FlattenRecursive(_outline.get(), 0);
}

void OutlineViewer::FlattenRecursive(
    const Document::OutlineItem* item, int depth) {
  if (item == nullptr) {
    return;
  }

  _lines.push_back(Line());
  const int line_num = _lines.size() - 1;

  _lines[line_num].OutlineItem = item;
  for (int i = 0; i < depth; ++i) {
    _lines[line_num].Label += "| ";
  }
  if (item->GetNumChildren()) {
    _all_expandable_items.insert(item);
    if (_expanded_items.count(item)) {
      _lines[line_num].Label += '+';
      for (int i = 0; i < item->GetNumChildren(); ++i) {
        FlattenRecursive(item->GetChild(i), depth + 1);
      }
    } else {
      _lines[line_num].Label += '-';
    }
  } else {
      _lines[line_num].Label += ' ';
  }
  _lines[line_num].Label += ' ';
  _lines[line_num].Label += item->GetTitle();
}

void OutlineViewer::Render() {
  WINDOW* const window = GetWindow();
  const int num_lines_to_display = std::min(
      getmaxy(window), static_cast<int>(_lines.size() - _first_index));
  for (int y = 0; y < num_lines_to_display; ++y) {
    const int line = _first_index + y;
    if (line == _selected_index) {
      wattron(window, A_STANDOUT);
    }
    mvwaddstr(window, y, 0, _lines[line].Label.c_str());
    wclrtoeol(window);
    if (line == _selected_index) {
      wattroff(window, A_STANDOUT);
    }
  }
  wclrtobot(window);
  wrefresh(window);
}

void OutlineViewer::ProcessKey(int key) {
  const Document::OutlineItem* selected_item =
      _lines[_selected_index].OutlineItem;
  const Document::OutlineItem* first_item =
      _lines[_first_index].OutlineItem;

  if (_key_processing_mode == REGULAR) {
    switch (key) {
      case '\t':
      case 'q':
      case 27:
        ExitEventLoop();
        break;
      case 'j':
      case KEY_DOWN:
        ++_selected_index;
        break;
      case 'k':
      case KEY_UP:
        --_selected_index;
        break;
      case KEY_NPAGE:
        _selected_index += getmaxy(GetWindow());
        break;
      case KEY_PPAGE:
        _selected_index -= getmaxy(GetWindow());
        break;
      case ' ':
        if (selected_item->GetNumChildren()) {
          if (_expanded_items.count(selected_item)) {
            _expanded_items.erase(selected_item);
          } else {
            _expanded_items.insert(selected_item);
          }
          Flatten();
        }
        break;
      case '\n':
      case '\r':
      case KEY_ENTER:
      case 'g':
        _selected_item = selected_item;
        ExitEventLoop();
        break;
      case 'z':
        _key_processing_mode = FOLD;
        break;
      default:
        break;
    }
  } else if (_key_processing_mode == FOLD) {
    switch (key) {
      case 'R':
      case 'r':
        _expanded_items = _all_expandable_items;
        Flatten();
        break;
      case 'M':
      case 'm':
        _expanded_items.clear();
        _expanded_items.insert(_outline.get());
        Flatten();
        break;
      default:
        break;
    }
    _selected_index = 0;
    for (unsigned int i = 0; i < _lines.size(); ++i) {
      if (_lines[i].OutlineItem == selected_item) {
        _selected_index = i;
        break;
      }
    }
    _first_index = 0;
    for (unsigned int i = 0; i < _lines.size(); ++i) {
      if (_lines[i].OutlineItem == first_item) {
        _first_index = i;
        break;
      }
    }
    _key_processing_mode = REGULAR;
  } else {
    // Undefined processing mode.
    assert(_key_processing_mode == REGULAR || _key_processing_mode == FOLD);
  }

  _selected_index = std::max(0, std::min(static_cast<int>(_lines.size() - 1),
      _selected_index));
  if (_selected_index < _first_index) {
    _first_index = _selected_index;
  } else if (_selected_index >= _first_index + getmaxy(GetWindow())) {
    _first_index = _selected_index - getmaxy(GetWindow()) + 1;
  }
}
