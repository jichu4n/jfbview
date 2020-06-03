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

// A simple tool to search for text in PDF documents.

#include <curses.h>
#include <getopt.h>

#include <climits>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "fitz_document.hpp"
#include "pdf_document.hpp"

namespace {

struct Options {
  int LineWidth;
  std::string FilePath;
  std::unique_ptr<std::string> FilePassword;
  std::string SearchString;
};

// Help text printed by --help or -h.
const char* HELP_STRING =
    "Search for a string in a PDF document.\n"
    "\n"
    "Usage: jpdfgrep [OPTIONS] FILE SEARCH_STRING\n"
    "\n"
    "Options:\n"
    "\t--help, -h            Show this message.\n"
    "\t--password=xx, -P xx  Unlock PDF document with the given password.\n"
    "\t--width=N, -w N       Specify result line width. The default is to\n"
    "\t                      autodetect terminal width.\n";

void ParseCommandLine(int argc, char* argv[], Options* options) {
  // Command line options.
  static const option LongFlags[] = {
      {"help", false, nullptr, 'h'},
      {"password", true, nullptr, 'P'},
      {"width", true, nullptr, 'w'},
      {0, 0, 0, 0},
  };
  static const char* ShortFlags = "hP:w:";

  options->LineWidth = 0;
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
      case 'w':
        if (sscanf(optarg, "%d", &(options->LineWidth)) < 1) {
          fprintf(stderr, "Invalid line width \"%s\"\n", optarg);
          exit(EXIT_FAILURE);
        }
        break;
      default:
        fprintf(stderr, "Try \"-h\" for help.\n");
        exit(EXIT_FAILURE);
    }
  }
  if (optind >= argc) {
    fprintf(stderr, "No file specified. Try \"-h\" for help.\n");
    exit(EXIT_FAILURE);
  } else if (optind == argc - 1) {
    fprintf(stderr, "No search string specified. Try \"-h\" for help.\n");
    exit(EXIT_FAILURE);
  } else if (optind < argc - 2) {
    fprintf(stderr, "Too many arguments. Try \"-h\" for help.\n");
    exit(EXIT_FAILURE);
  } else {
    options->FilePath = argv[optind];
    options->SearchString = argv[optind + 1];
  }
  if (options->SearchString.empty()) {
    fprintf(stderr, "Empty search string specified. Try \"-h\" for help.\n");
    exit(EXIT_FAILURE);
  }
}

}  // namespace

int JpdfgrepMain(int argc, char* argv[]) {
  Options options;
  ParseCommandLine(argc, argv, &options);

  std::unique_ptr<Document> document(
#ifdef JFBVIEW_ENABLE_LEGACY_PDF_IMPL
      PDFDocument::Open(options.FilePath, options.FilePassword.get())
#else
      FitzDocument::Open(options.FilePath, options.FilePassword.get())
#endif
  );
  if (!document) {
    fprintf(stderr, "Failed to open \"%s\"\n", options.FilePath.c_str());
    return EXIT_FAILURE;
  }

  if (options.LineWidth <= 0) {
    // Get terminal window width.
    initscr();
    options.LineWidth = COLS;
    endwin();
  }

  const Document::SearchResult& result =
      document->Search(options.SearchString, 0, options.LineWidth, INT_MAX);

  for (const Document::SearchHit& hit : result.SearchHits) {
    std::ostringstream buffer;
    buffer << (hit.Page + 1) << ": " << hit.ContextText;
    fprintf(stdout, "%s\n", buffer.str().substr(0, options.LineWidth).c_str());
  }

  return EXIT_SUCCESS;
}
