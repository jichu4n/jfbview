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

// This file declares the search view.

#ifndef SEARCH_VIEW_HPP
#define SEARCH_VIEW_HPP

#include <form.h>
#include <string>
#include <vector>
#include "document.hpp"
#include "ui_view.hpp"

// Search view class. This class stores the search string and search results
// between invocations.
class SearchView : public UIView {
 public:
  // Constructs an instance of SearchView that searches through the given
  // document. Does not take ownership. The document must be valid throughout
  // the lifetime of this object.
  explicit SearchView(Document* document);
  virtual ~SearchView();

  // Displays the search view and enters the event loop. If the user selected a
  // page to jump to, returns the selected page. Otherwise, returns a negative
  // number.
  int Run();

 protected:
  // See UIView.
  void Render() override;

 private:
  // Search prompt string.
  static const char* const _SEARCH_PROMPT;
  // Search progress string.
  static const char* const _SEARCH_PROGRESS_PREFIX;
  // Progress filler char.
  enum { _SEARCH_PROGRESS_CHAR = '.' };
  // How many filler chars to display at most.
  enum { _MAX_NUM_SEARCH_PROGRESS_CHARS = 3 };
  // Delay between two updates to the search progress, in milliseconds.
  enum { _SEARCH_PROGRESS_UPDATE_DELAY_MS = 250 };
  // Padding inside the search progress window.
  enum { _SEARCH_PROGRESS_PADDING = 1 };
  // String to display when there are no results.
  static const char* const _NO_RESULTS_PROMPT;
  // Padded width of page numbers shown in search results.
  enum { _PAGE_NUMBER_WIDTH = 6 };
  // Page number prefix.
  static const char* const _PAGE_NUMBER_PREFIX;
  // The maxinum number of search results per search is the product of the
  // result window height and this factor.
  enum { _MAX_NUM_SEARCH_HITS_FACTOR = 2 };
  // The maximum number of search results will be rounded for display to
  // multiples of the following number.
  enum { _MAX_NUM_SEARCH_HITS_DISPLAY_ROUNDING = 100 };
  // The context text length to request and display.
  int _context_text_length;

  // The document to search.
  Document* const _document;
  // NCURSES Window containing the search form.
  WINDOW* _search_window;
  // NCURSES search string form.
  FORM*_search_form;
  // Search string NCURSES field.
  FIELD* _search_string_field;
  // The list of all NCURSES fields in the form.
  std::vector<FIELD*> _fields;
  // The current position of the cursor in the search string field.
  int _search_string_field_cursor_position;
  // The current displayed search result.
  std::unique_ptr<Document::SearchResult> _result;
  // The result window.
  WINDOW* _result_window;
  // The status window.
  WINDOW* _status_window;
  // Currently first visible search hit index.
  int _first_index;
  // Currently highlighted search hit index.
  int _selected_index;

  // The page the user desires to go to.
  int _selected_page;

  // Key processing modes.
  enum KeyProcessingMode {
    REGULAR_MODE,              // Regular mode.
    SEARCH_STRING_FIELD_MODE,  // Focus inside search string field.
  };
  // Key processors.
  void ProcessKeyRegularMode(int key);
  void ProcessKeySearchStringFieldMode(int key);

  // Switch focus to search string form.
  void SwitchToSearchStringField();
  // Switch focus to search result.
  void SwitchToSearchResult();
  // Runs search and displays results.
  void Search();
  // Returns whether we have searched all pages.
  bool HasSearchedAllPages();
  // The maximum search hit index value.
  int GetMaxIndex() { return _result ? _result->SearchHits.size() - 1 : 0; }

  // Returns the current text in the search string field.
  std::string GetSearchString();
};

#endif
