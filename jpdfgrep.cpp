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

// A simple tool to search for text in PDF documents.

#include <curses.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "pdf_document.hpp"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "No file specified" << std::endl;
    return 1;
  }
  if (argc < 3) {
    std::cerr << "No search string specified" << std::endl;
    return 1;
  }

  const std::string file_path = argv[1];
  std::unique_ptr<PDFDocument> document(PDFDocument::Open(file_path));
  if (!document) {
    std::cerr << "Failed to open " << file_path;
    return 1;
  }

  // Get terminal window width.
  initscr();
  const int line_width = COLS;
  endwin();

  const std::string search_string = argv[2];
  const Document::SearchResult& result = document->Search(
      search_string, 0, line_width, INT_MAX);

  for (const Document::SearchHit& hit : result.SearchHits) {
    std::ostringstream buffer;
    buffer << (hit.Page + 1) << ": " << hit.ContextText;
    std::cout << buffer.str().substr(0, line_width) << std::endl;
  }

  return 0;
}
