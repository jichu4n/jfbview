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

// A simple tool to print out text in PDF documents.

#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "pdf_document.hpp"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "No file specified" << std::endl;
    return 1;
  }
  const std::string file_path = argv[1];
  std::unique_ptr<PDFDocument> document(PDFDocument::Open(file_path));
  if (!document) {
    std::cerr << "Failed to open " << file_path;
    return 1;
  }

  std::vector<int> pages;
  if (argc >= 3) {
    for (int i = 2; i < argc; ++i) {
      int page;
      std::istringstream in(argv[i]);
      in >> page;
      --page;
      if (page < 0 || page > document->GetNumPages()) {
        std::cerr << "Invalid page number " << (page + 1)
                  << ". Please specify a number between 1 and "
                  << document->GetNumPages() << std::endl;
        return 1;
      }
      pages.push_back(page);
    }
  } else {
    for (int i = 0; i < document->GetNumPages(); ++i) {
      pages.push_back(i);
    }
  }

  for (int page : pages) {
    const std::string& page_text = document->GetPageText(page);
    std::cout << page_text << std::endl;
  }

  return 0;
}
