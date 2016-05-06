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

// This file implements the search view.

#include "search_view.hpp"

#include <curses.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include "string_utils.hpp"
#include "cpp_compat.hpp"

using std::placeholders::_1;

const char* const SearchView::_SEARCH_PROMPT = "Search: ";
const char* const SearchView::_SEARCH_PROGRESS_PREFIX = "Searching";
const char* const SearchView::_NO_RESULTS_PROMPT = "No results found.";
const char* const SearchView::_PAGE_NUMBER_PREFIX = "p";

SearchView::SearchView(Document* document)
    : UIView({{
          REGULAR_MODE,
          std::bind(&SearchView::ProcessKeyRegularMode, this, _1),
      }, {
          SEARCH_STRING_FIELD_MODE,
          std::bind(&SearchView::ProcessKeySearchStringFieldMode, this, _1),
      }}),
      _document(document),
      _search_string_field_cursor_position(0) {
  assert(_document != nullptr);

  WINDOW* const window = GetWindow();
  int window_width, window_height;
  getmaxyx(window, window_height, window_width);

  // 1. Construct search form.
  const int search_string_form_left = strlen(_SEARCH_PROMPT);
  const int search_string_form_width = window_width - search_string_form_left;

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
  _search_form = new_form(_fields.data());
  assert(_search_form != nullptr);
  set_form_win(_search_form, _search_window);
  set_form_sub(_search_form, _search_window);

  // 2. Construct result and status windows.
  _result_window = derwin(
      window, window_height - 1 - 1 - 1, window_width, 1 + 1, 0);
  _context_text_length =
      window_width - strlen(_PAGE_NUMBER_PREFIX) - _PAGE_NUMBER_WIDTH;
  _status_window = derwin(
      window, 1, window_width, window_height - 1, 0);
  wbkgdset(_status_window, A_STANDOUT);
}

SearchView::~SearchView() {
  free_form(_search_form);
  assert(_fields.back() == nullptr);
  _fields.pop_back();
  for (FIELD* field : _fields) {
    free_field(field);
  }
  delwin(_status_window);
  delwin(_result_window);
  delwin(_search_window);
}

int SearchView::Run() {
  WINDOW* const window = GetWindow();
  wclear(window);

  mvwaddstr(window, 0, 0, _SEARCH_PROMPT);
  mvwhline(window, 1, 0, 0, INT_MAX);

  int r = post_form(_search_form);
  assert(r == E_OK);
  // set_current_field(_search_form, _search_string_field);
  SwitchToSearchStringField();

  _selected_page = -1;

  EventLoop(SEARCH_STRING_FIELD_MODE);

  unpost_form(_search_form);
  curs_set(false);
  wrefresh(window);

  return _selected_page;
}

void SearchView::Render() {
  WINDOW* const window = GetWindow();
  int result_window_width, result_window_height;
  getmaxyx(_result_window, result_window_height, result_window_width);

  if (_result) {
    if (_result->SearchHits.empty()) {
      wclear(_result_window);
      mvwaddstr(
          _result_window,
          result_window_height / 2,
          (result_window_width - strlen(_NO_RESULTS_PROMPT)) / 2,
          _NO_RESULTS_PROMPT);
    } else {
      // 1. Draw search hits.
      const int last_index = std::min(
          _first_index + result_window_height,
          static_cast<int>(_result->SearchHits.size()) - 1);
      for (int i = 0; i + _first_index <= last_index; ++i) {
        const Document::SearchHit& hit = _result->SearchHits[i + _first_index];
        const bool is_selected_hit =
            (i + _first_index == _selected_index) &&
            (GetKeyProcessingMode() == REGULAR_MODE);
        if (is_selected_hit) {
          wattron(_result_window, A_STANDOUT);
        }

        // 1.1. Page number.
        wattron(_result_window, A_BOLD);
        std::ostringstream buffer;
        buffer <<_PAGE_NUMBER_PREFIX
               << std::setw(_PAGE_NUMBER_WIDTH) << std::left << hit.Page;
        mvwaddstr(
            _result_window,
            i, 0, buffer.str().c_str());
        wattroff(_result_window, A_BOLD);

        // 1.2. Context.
        int offset = 0;
        waddnstr(
            _result_window,
            hit.ContextText.c_str() + offset,
            hit.SearchStringPosition);
        offset += hit.SearchStringPosition;
        const int search_string_length = std::min(
            _context_text_length - offset,
            static_cast<int>(_result->SearchString.length()));
        wattron(_result_window, A_UNDERLINE);
        wattron(_result_window, A_BOLD);
        waddnstr(
            _result_window,
            hit.ContextText.c_str() + offset,
            search_string_length);
        wattroff(_result_window, A_BOLD);
        wattroff(_result_window, A_UNDERLINE);
        offset += search_string_length;
        assert(_context_text_length >= offset);
        waddnstr(
            _result_window,
            hit.ContextText.c_str() + offset,
            _context_text_length - offset);

        wclrtoeol(_result_window);

        if (is_selected_hit) {
          wattroff(_result_window, A_STANDOUT);
        }
      }
      wclrtobot(_result_window);

      // 2. Draw status.
      std::ostringstream buffer;
      buffer << (_selected_index + 1) << " of ";
      if (HasSearchedAllPages()) {
        buffer << _result->SearchHits.size();
      } else {
        buffer << (
            (_result->SearchHits.size() /
             _MAX_NUM_SEARCH_HITS_DISPLAY_ROUNDING) *
            _MAX_NUM_SEARCH_HITS_DISPLAY_ROUNDING) << "+";
      }
      buffer << " results";
      if (!HasSearchedAllPages()) {
        buffer << " (scroll to see all)";
      }
      mvwaddstr(_status_window, 0, 0, buffer.str().c_str());
      wclrtoeol(_status_window);
    }
    wrefresh(_result_window);
    wrefresh(_status_window);
  }

  wrefresh(window);
  /*
  {
    std::ostringstream out;
    out << _search_string_field_cursor_position << "          ";
    mvwaddstr(window, 1, 0, out.str().c_str());
    wrefresh(window);
  }
  */
}

void SearchView::ProcessKeySearchStringFieldMode(int key) {
  const std::string& search_string = GetSearchString();

  switch (key) {
    case 27:
      ExitEventLoop();
      break;
    case '\n':
    case '\r':
    case KEY_ENTER:
      Search();
      break;
    case KEY_BACKSPACE:
    case 127:
      if ((_search_string_field_cursor_position <= 0) &&
          search_string.empty()) {
        ExitEventLoop();
      } else {
        form_driver(_search_form, REQ_DEL_PREV);
      }
      break;
    case KEY_LEFT:
      if (_search_string_field_cursor_position > 0) {
        form_driver(_search_form, REQ_PREV_CHAR);
      }
      break;
    case KEY_RIGHT:
      if (_search_string_field_cursor_position <
          static_cast<int>(search_string.length())) {
        form_driver(_search_form, REQ_NEXT_CHAR);
      }
      break;
    case KEY_DC:
      form_driver(_search_form, REQ_DEL_CHAR);
      break;
    case KEY_HOME:
      form_driver(_search_form, REQ_BEG_FIELD);
      break;
    case KEY_END:
      form_driver(_search_form, REQ_END_FIELD);
      break;
    case '\t':
    case KEY_DOWN:
    case KEY_NPAGE:
      if (_result && !_result->SearchHits.empty()) {
        SwitchToSearchResult();
      }
      break;
    default:
      form_driver(_search_form, key);
      /*
      {
        WINDOW* const window = GetWindow();
        std::ostringstream out;
        // out << key;
        // out << "'" << search_string << "'";
        mvwaddstr(window, 1, 0, out.str().c_str());
        wrefresh(window);
      }
      */
      break;
  }
  int cursor_y;
  getyx(_search_window, cursor_y, _search_string_field_cursor_position);
  assert(cursor_y == 0);
}

void SearchView::ProcessKeyRegularMode(int key) {
  const int result_window_height = getmaxy(_result_window);

  switch (key) {
    case 'q':
    case 27:
      ExitEventLoop();
      break;
    case '\t':
    case '/':
      SwitchToSearchStringField();
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
      _selected_index += result_window_height;
      break;
    case KEY_PPAGE:
      if (_selected_index <= 0) {
        --_selected_index;
      } else {
        _selected_index -= std::min(_selected_index, result_window_height);
      }
      break;
    case ' ':
    case '\n':
    case '\r':
    case KEY_ENTER:
    case 'g':
      if (_result && !_result->SearchHits.empty()) {
        assert(_selected_index >= 0);
        assert(_selected_index < static_cast<int>(_result->SearchHits.size()));
        _selected_page = _result->SearchHits[_selected_index].Page;
        ExitEventLoop();
      }
      break;
    default:
      break;
  }

  if (_selected_index < 0) {
    SwitchToSearchStringField();
    _selected_index = 0;
  } else if ((_selected_index > GetMaxIndex()) && !HasSearchedAllPages()) {
    Search();
  }
  _selected_index = std::max(0, std::min(GetMaxIndex(), _selected_index));
  if (_selected_index < _first_index) {
    _first_index = _selected_index;
  } else if (_selected_index >= _first_index + result_window_height) {
    _first_index = _selected_index - result_window_height + 1;
  }
}

void SearchView::SwitchToSearchStringField() {
  SwitchKeyProcessingMode(SEARCH_STRING_FIELD_MODE);

  curs_set(true);
  // There doesn't seem to be a way to set the cursor position in a field.
  // Yikes.
  form_driver(_search_form, REQ_BEG_FIELD);
  for (int i = 0; i < _search_string_field_cursor_position; ++i) {
    form_driver(_search_form, REQ_NEXT_CHAR);
  }
}

void SearchView::SwitchToSearchResult() {
  SwitchKeyProcessingMode(REGULAR_MODE);

  curs_set(false);
}

void SearchView::Search() {
  const std::string& search_string = Trim(GetSearchString());
  if (search_string.empty()) {
    return;
  }

  SwitchToSearchResult();

  WINDOW* const window = GetWindow();
  int window_width, window_height;
  getmaxyx(window, window_height, window_width);
  int result_window_height, result_window_width;
  getmaxyx(_result_window, result_window_height, result_window_width);
  const int max_num_search_hits = std::max(
      result_window_height * _MAX_NUM_SEARCH_HITS_FACTOR,
      static_cast<int>(_MAX_NUM_SEARCH_HITS_DISPLAY_ROUNDING));

  // 0. If the search string hasn't changed, continue last search. Else, start
  // afresh.
  int search_start_page;
  if (_result && (_result->SearchString == search_string)) {
    search_start_page = _result->LastSearchedPage + 1;
  } else {
    _result.reset();
    _selected_index = 0;
    _first_index = 0;
    search_start_page = 0;
  }

  // 1. Start search in new thread.
  std::mutex result_mutex;
  std::unique_lock<std::mutex> main_thread_result_lock(result_mutex);
  std::condition_variable result_cond;
  std::unique_ptr<Document::SearchResult> result;

  std::thread search_thread([&]() {
      result = std::make_unique<Document::SearchResult>(_document->Search(
          search_string,
          search_start_page,
          _context_text_length,
          max_num_search_hits));
      std::unique_lock<std::mutex> search_thread_result_lock(result_mutex);
      result_cond.notify_all();
  });

  // 2. Construct progress window.
  const int progress_window_width =
      1 + _SEARCH_PROGRESS_PADDING +
      strlen(_SEARCH_PROGRESS_PREFIX) + _MAX_NUM_SEARCH_PROGRESS_CHARS +
      _SEARCH_PROGRESS_PADDING + 1;
  const int progress_window_height =
      1 + _SEARCH_PROGRESS_PADDING + 1 + _SEARCH_PROGRESS_PADDING + 1;
  const int progress_window_x = (window_width - progress_window_width) / 2;
  const int progress_window_y = (window_height - progress_window_height) / 2;
  WINDOW* progress_window = derwin(
      window,
      progress_window_height, progress_window_width,
      progress_window_y, progress_window_x);
  wbkgdset(progress_window, A_STANDOUT);
  wclear(progress_window);
  box(progress_window, 0, 0);
  mvwaddstr(
      progress_window,
      1 + _SEARCH_PROGRESS_PADDING, 1 + _SEARCH_PROGRESS_PADDING,
      _SEARCH_PROGRESS_PREFIX);
  int search_progress_chars_x, search_progress_chars_y;
  getyx(
      progress_window,
      search_progress_chars_y, search_progress_chars_x);

  // 3. Block until search result is available.
  for (int num_progress_chars = 0; ; ++num_progress_chars) {
    // 3.1. Draw progress chars.
    num_progress_chars =
        num_progress_chars % (_MAX_NUM_SEARCH_PROGRESS_CHARS + 1);
    wmove(
        progress_window,
        search_progress_chars_y, search_progress_chars_x);
    for (int i = 1; i <= _MAX_NUM_SEARCH_PROGRESS_CHARS; ++i) {
      const char c = i <= num_progress_chars ? _SEARCH_PROGRESS_CHAR : ' ';
      waddch(progress_window, c);
    }
    wrefresh(progress_window);

    // 3.2. Wait for search result.
    const std::cv_status r = result_cond.wait_for(
        main_thread_result_lock,
        std::chrono::milliseconds(_SEARCH_PROGRESS_UPDATE_DELAY_MS));
    if (r == std::cv_status::no_timeout) {
      break;
    }
  }
  assert(static_cast<bool>(result));
  // join() must be called before thread object destruction. Since the search
  // thread should have terminated, this should not block.
  search_thread.join();

  // 4. Clean up progress window.
  wclear(progress_window);
  delwin(progress_window);
  /*
  {
    std::ostringstream out;
    out << "Found " << result->SearchHits.size() << " results";
    mvwaddstr(window, window_height - 1, 0, out.str().c_str());
    wclrtoeol(window);
  }
  */

  // 5. Reset or merge search results.
  if (_result) {
    assert(_result->SearchString == result->SearchString);
    _result->LastSearchedPage = result->LastSearchedPage;
    _result->SearchHits.insert(
        _result->SearchHits.end(),
        result->SearchHits.begin(), result->SearchHits.end());
  } else {
    _result = std::move(result);
  }
}

std::string SearchView::GetSearchString() {
  // The value of the buffer a field is not available until we call form_driver
  // with REQ_VALIDATION.
  form_driver(_search_form, REQ_VALIDATION);
  // The valued returned will be padded to the width of the field (W.T.F.), so
  // we have to rtrim it.
  return TrimRight(field_buffer(_search_string_field, 0));
}

bool SearchView::HasSearchedAllPages() {
  return _result && (_result->LastSearchedPage >= _document->GetNumPages() - 1);
}
