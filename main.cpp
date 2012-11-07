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

// Main program file.

#include "framebuffer.hpp"
#include "pdf_document.hpp"
#include "viewer.hpp"
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <string>

// Main program state.
struct State: public Viewer::Config {
  // Input file.
  std::string FilePath;
  // Framebuffer device.
  std::string FramebufferDevice;

  // Default state.
  State()
      : Config(), FilePath(""),
        FramebufferDevice(Framebuffer::DEFAULT_FRAMEBUFFER_DEVICE) {
  }
};

// Command line options.
static option LongFlags[] = {
    { "fb", true, NULL, 'f' },
    { "page", true, NULL, 'p' },
    { "zoom", true, NULL, 'z' },
    { "rotation", true, NULL, 'r' },
    { 0, 0, 0, 0 },
};
static const char *ShortFlags = "f:p:z:r:";


int main(int argc, char *argv[]) {
  // Main program state.
  State state;

  // 1. Scan command line options.
  for (;;) {
    int opt_char = getopt_long(argc, argv, ShortFlags, LongFlags, NULL);
    if (opt_char == -1) {
      break;
    }
    switch (opt_char) {
     case 'f':
       state.FramebufferDevice = optarg;
       break;
     case 'p':
      if (sscanf(optarg, "%d", &(state.Page)) < 1) {
        fprintf(stderr, "Invalid page number \"%s\"\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;
     case 'z':
      if (sscanf(optarg, "%f", &(state.Zoom)) < 1) {
        fprintf(stderr, "Invalid zoom ratio \"%s\"\n", optarg);
        exit(EXIT_FAILURE);
      }
      state.Zoom /= 100.0f;
      break;
     case 'r':
      if (sscanf(optarg, "%d", &(state.Rotation)) < 1) {
        fprintf(stderr, "Invalid rotation degree \"%s\"\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;
     default:
      exit(EXIT_FAILURE);
    }
  }
  if (optind == argc) {
    fprintf(stderr, "No file specified.\n");
    exit(EXIT_FAILURE);
  } else if (optind < argc - 1) {
    fprintf(stderr, "Please specify exactly one input file.\n");
    exit(EXIT_FAILURE);
  }

  // 2. Initialization.
  Document *doc = PDFDocument::Open(argv[optind]);
  if (doc == NULL) {
    fprintf(stderr, "Failed to open document \"%s\".\n", argv[optind]);
    exit(EXIT_FAILURE);
  }
  Framebuffer *fb = Framebuffer::Open(state.FramebufferDevice);
  if (fb == NULL) {
    fprintf(stderr, "Failed to initialize framebuffer device \"%s\".\n",
            state.FramebufferDevice.c_str());
    delete doc;
    exit(EXIT_FAILURE);
  }
  Viewer viewer(doc, fb, state);

  delete fb;
  delete doc;

  return EXIT_SUCCESS;
}

