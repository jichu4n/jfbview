/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012-2014 Chuan Ji <jichuan89@gmail.com>                   *
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

// This file implements the search view.

#include "search_view.hpp"

#include <curses.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <sstream>

namespace {

// Trim leading whitespace.
std::string ltrim(const std::string& s) {
  std::string r(s);
  r.erase(r.begin(), std::find_if(
        r.begin(), r.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  return r;
}

// Trim trailing whitespace.
std::string rtrim(const std::string& s) {
  std::string r(s);
  r.erase(
      std::find_if(
          r.rbegin(),
          r.rend(),
          std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
      r.end());
  return r;
}


}  // namespace

const char* const SearchView::_SEARCH_PROMPT = "Search: ";

SearchView::SearchView(Document* document)
    : _document(document) {
  assert(_document != nullptr);

  WINDOW* const window = GetWindow();

  const int search_string_form_left = strlen(_SEARCH_PROMPT);
  const int search_string_form_width =
      getmaxx(window) - search_string_form_left;

  _search_window = derwin(
      window, 1, search_string_form_width, 0, search_string_form_left);

  _search_string_field = new_field(
      1, search_string_form_width,
      0, 0,
      0,
      0);
  assert(_search_string_field != nullptr);
  set_field_back(_search_string_field, A_STANDOUT);
  field_opts_off(_search_string_field, O_AUTOSKIP | O_BLANK);
  _fields.push_back(_search_string_field);

  _fields.push_back(nullptr);
  _form = new_form(_fields.data());
  assert(_form != nullptr);
  set_form_win(_form, _search_window);
  set_form_sub(_form, _search_window);
}

SearchView::~SearchView() {
  free_form(_form);
  assert(_fields.back() == nullptr);
  _fields.pop_back();
  for (FIELD* field : _fields) {
    free_field(field);
  }
  delwin(_search_window);
}

int SearchView::Run() {
  WINDOW* const window = GetWindow();
  curs_set(true);
  wclear(window);

  mvwaddstr(window, 0, 0, _SEARCH_PROMPT);

  int r = post_form(_form);
  assert(r == E_OK);
  set_current_field(_form, _search_string_field);

  EventLoop();

  unpost_form(_form);
  curs_set(false);

  return -1;
}

void SearchView::Render() {
  WINDOW* const window = GetWindow();
  wrefresh(window);
}

void SearchView::ProcessKey(int key) {
  const std::string& search_string = GetSearchString();
  int cursor_x, cursor_y;
  getyx(_search_window, cursor_y, cursor_x);

  switch (key) {
    case '/':
    case 'q':
    case 27:
      ExitEventLoop();
      break;
    case KEY_BACKSPACE:
    case 127:
      form_driver(_form, REQ_DEL_PREV);
      break;
    case KEY_LEFT:
      if (cursor_x > 0) {
        form_driver(_form, REQ_PREV_CHAR);
      }
      break;
    case KEY_RIGHT:
      if (cursor_x < search_string.size()) {
        form_driver(_form, REQ_NEXT_CHAR);
      }
      break;
    case KEY_DC:
      form_driver(_form, REQ_DEL_CHAR);
      break;
    case KEY_HOME:
      form_driver(_form, REQ_BEG_FIELD);
      break;
    case KEY_END:
      form_driver(_form, REQ_END_FIELD);
      break;
    default:
      form_driver(_form, key);
      /*
      {
        WINDOW* const window = GetWindow();
        std::ostringstream out;
        // out << key;
        out << "'" << search_string << "'";
        mvwaddstr(window, 1, 0, out.str().c_str());
        wrefresh(window);
      }
      */
      break;
  }
}

std::string SearchView::GetSearchString() {
  // The value of the buffer a field is not available until we call form_driver
  // with REQ_VALIDATION.
  form_driver(_form, REQ_VALIDATION);
  // The valued returned will be padded to the width of the field (W.T.F.), so
  // we have to rtrim it.
  return rtrim(field_buffer(_search_string_field, 0));
}
