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
  void ProcessKey(int key) override;

 private:
  // Returns the current text in the search string field.
  std::string GetSearchString();

  // Search prompt string.
  static const char* const _SEARCH_PROMPT;

  // The document to search.
  Document* const _document;
  // NCURSES Window containing the search form.
  WINDOW* _search_window;
  // NCURSES search form.
  FORM* _form;
  // Search string NCURSES field.
  FIELD* _search_string_field;
  // The list of all NCURSES fields in the form.
  std::vector<FIELD*> _fields;
};

#endif
