/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012-2020 Chuan Ji                                         *
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

#include <getopt.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "pdf_document.hpp"

namespace {

struct Options {
  std::string FilePath;
  std::unique_ptr<std::string> FilePassword;
  std::vector<int> Pages;
};

// Help text printed by --help or -h.
const char* HELP_STRING =
    "Extract and print the text content in a PDF document.\n"
    "\n"
    "Usage: jpdfcat [OPTIONS] FILE [PAGE]...\n"
    "\n"
    "Options:\n"
    "\t--help, -h            Show this message.\n"
    "\t--password=xx, -P xx  Unlock PDF document with the given password.\n";

void ParseCommandLine(int argc, char* argv[], Options* options) {
  // Command line options.
  static const option LongFlags[] = {
      {"help", false, nullptr, 'h'},
      {"password", true, nullptr, 'P'},
      {0, 0, 0, 0},
  };
  static const char* ShortFlags = "hP:";

  for (;;) {
    int opt_char = getopt_long(argc, argv, ShortFlags, LongFlags, nullptr);
    if (opt_char == -1) {
      break;
    }
    switch (opt_char) {
      case 'h':
        fprintf(stdout, "%s", HELP_STRING);
        exit(EXIT_FAILURE);
        break;
      case 'P':
        options->FilePassword = std::make_unique<std::string>(optarg);
        break;
      default:
        fprintf(stderr, "Try \"-h\" for help.\n");
        exit(EXIT_FAILURE);
    }
  }
  if (optind >= argc) {
    fprintf(stderr, "No file specified. Try \"-h\" for help.\n");
    exit(EXIT_FAILURE);
  } else {
    options->FilePath = argv[optind];
  }
  for (int i = optind + 1; i < argc; ++i) {
    int page;
    if (sscanf(argv[i], "%d", &page) < 1) {
      fprintf(stderr, "Invalid page number \"%s\"\n", optarg);
      exit(EXIT_FAILURE);
    }
    options->Pages.push_back(page);
  }
}

}  // namespace

int JpdfcatMain(int argc, char* argv[]) {
  Options options;
  ParseCommandLine(argc, argv, &options);

  std::unique_ptr<PDFDocument> document(
      PDFDocument::Open(options.FilePath, options.FilePassword.get()));
  if (!document) {
    fprintf(stderr, "Failed to open \"%s\"\n", options.FilePath.c_str());
    return EXIT_FAILURE;
  }

  if (options.Pages.empty()) {
    for (int i = 0; i < document->GetNumPages(); ++i) {
      options.Pages.push_back(i);
    }
  } else {
    for (int& page : options.Pages) {
      if (page < 1 || page > document->GetNumPages()) {
        fprintf(
            stderr,
            "Invalid page number %d. "
            "Please specify a number between 1 and %d.\n",
            page, document->GetNumPages());
        return EXIT_FAILURE;
      }
      --page;
    }
  }

  for (int page : options.Pages) {
    const std::string& page_text = document->GetPageText(page);
    fprintf(stdout, "%s\n", page_text.c_str());
  }

  return EXIT_SUCCESS;
}
