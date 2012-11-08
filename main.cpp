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

#include "command.hpp"
#include "framebuffer.hpp"
#include "pdf_document.hpp"
#include "viewer.hpp"
#include <curses.h>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <algorithm>
#include <string>

// Main program state.
struct State: public Viewer::State {
  // If true, exit main event loop.
  bool Exit;
  // If true (default), requires refresh after current command.
  bool Render;

  // Input file.
  std::string FilePath;
  // Framebuffer device.
  std::string FramebufferDevice;

  // Default state.
  State()
      : Viewer::State(), Exit(false), Render(true), FilePath(""),
        FramebufferDevice(Framebuffer::DEFAULT_FRAMEBUFFER_DEVICE) {
  }
};


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                 COMMANDS                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

class ExitCommand: public Command {
 public:
  virtual void Execute(int repeat, State *state) {
    state->Exit = true;
  }
};

// Base class for move commands.
class MoveCommand: public Command {
 protected:
  // Returns how much to move by in a direction.
  int GetMoveSize(const State *state, bool horizontal) const {
    if (horizontal) {
      return state->ScreenWidth / 10;
    }
    return state->ScreenHeight / 10;
  }
};

class MoveDownCommand: public MoveCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    state->YOffset += RepeatOrDefault(repeat, 1) * GetMoveSize(state, false);
    if (state->YOffset + state->ScreenHeight >=
            state->PageHeight - 1 + GetMoveSize(state, false)) {
      if (++(state->Page) < state->PageCount) {
        state->YOffset = 0;
      }
    }
  }
};

class MoveUpCommand: public MoveCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    state->YOffset -= RepeatOrDefault(repeat, 1) * GetMoveSize(state, false);
    if (state->YOffset <= -GetMoveSize(state, false)) {
      if (--(state->Page) >= 0) {
        state->YOffset = INT_MAX;
      }
    }
  }
};

class MoveLeftCommand: public MoveCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    state->XOffset -= RepeatOrDefault(repeat, 1) * GetMoveSize(state, true);
  }
};

class MoveRightCommand: public MoveCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    state->XOffset += RepeatOrDefault(repeat, 1) * GetMoveSize(state, true);
  }
};

class ScreenDownCommand: public Command {
 public:
  virtual void Execute(int repeat, State *state) {
    state->YOffset += RepeatOrDefault(repeat, 1) * state->ScreenHeight;
    if (state->YOffset + state->ScreenHeight >=
            state->PageHeight - 1 + state->ScreenHeight) {
      if (++(state->Page) < state->PageCount) {
        state->YOffset = 0;
      }
    }
  }
};

class ScreenUpCommand: public Command {
 public:
  virtual void Execute(int repeat, State *state) {
    state->YOffset -= RepeatOrDefault(repeat, 1) * state->ScreenHeight;
    if (state->YOffset <= -state->ScreenHeight) {
      if (--(state->Page) >= 0) {
        state->YOffset = INT_MAX;
      }
    }
  }
};

// Base class for zoom commands.
class ZoomCommand: public Command {
 protected:
  // How much to zoom in/out by each time.
  static const float ZOOM_COEFFICIENT;
  // Sets zoom, preserving original screen center.
  void SetZoom(float zoom, State *state) {
    // Position in page of screen center, as fraction of page size.
    const float center_ratio_x =
        static_cast<float>(state->XOffset + state->ScreenWidth / 2) /
        static_cast<float>(state->PageWidth);
    const float center_ratio_y =
        static_cast<float>(state->YOffset + state->ScreenHeight / 2) /
        static_cast<float>(state->PageHeight);
    // Bound zoom.
    zoom = std::max(Viewer::MIN_ZOOM, std::min(Viewer::MAX_ZOOM, zoom));
    // Quotient of new and old zoom ratios.
    const float q = zoom / state->ActualZoom;
    // New page size after zoom change.
    const float new_page_width = roundf(
        static_cast<float>(state->PageWidth) * q);
    const float new_page_height = roundf(
        static_cast<float>(state->PageHeight) * q);
    // New center position within page after zoom change.
    const float new_center_x = roundf(new_page_width * center_ratio_x);
    const float new_center_y = roundf(new_page_height * center_ratio_y);
    // New offsets.
    state->XOffset = static_cast<int>(new_center_x) - state->ScreenWidth / 2;
    state->YOffset = static_cast<int>(new_center_y) - state->ScreenHeight / 2;
    // New zoom.
    state->Zoom = zoom;
  }
};
const float ZoomCommand::ZOOM_COEFFICIENT = 1.2f;

class ZoomInCommand: public ZoomCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    SetZoom(state->ActualZoom * RepeatOrDefault(repeat, 1) * ZOOM_COEFFICIENT,
            state);
  }
};

class ZoomOutCommand: public ZoomCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    SetZoom(state->ActualZoom * RepeatOrDefault(repeat, 1) / ZOOM_COEFFICIENT,
            state);
  }
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                               END COMMANDS                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


// Parses the command line, and stores settings in state. Crashes the program if
// the commnad line contains errors.
void ParseCommandLine(int argc, char *argv[], State *state) {
  // Tags for long options that don't have short option chars.
  enum {
    ZOOM_TO_WIDTH = 0x1000,
    ZOOM_TO_FIT,
    FB,
  };
  // Command line options.
  static const option LongFlags[] = {
      { "fb", true, NULL, FB },
      { "page", true, NULL, 'p' },
      { "zoom", true, NULL, 'z' },
      { "zoom_to_width", false, NULL, ZOOM_TO_WIDTH },
      { "zoom_to_fit", false, NULL, ZOOM_TO_FIT },
      { "rotation", true, NULL, 'r' },
      { 0, 0, 0, 0 },
  };
  static const char *ShortFlags = "p:z:r:";

  for (;;) {
    int opt_char = getopt_long(argc, argv, ShortFlags, LongFlags, NULL);
    if (opt_char == -1) {
      break;
    }
    switch (opt_char) {
     case FB:
       state->FramebufferDevice = optarg;
       break;
     case 'p':
      if (sscanf(optarg, "%d", &(state->Page)) < 1) {
        fprintf(stderr, "Invalid page number \"%s\"\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;
     case 'z':
      if (sscanf(optarg, "%f", &(state->Zoom)) < 1) {
        fprintf(stderr, "Invalid zoom ratio \"%s\"\n", optarg);
        exit(EXIT_FAILURE);
      }
      state->Zoom /= 100.0f;
      break;
     case ZOOM_TO_WIDTH:
      state->Zoom = Viewer::ZOOM_TO_WIDTH;
      break;
     case ZOOM_TO_FIT:
      state->Zoom = Viewer::ZOOM_TO_FIT;
      break;
     case 'r':
      if (sscanf(optarg, "%d", &(state->Rotation)) < 1) {
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
  } else {
    state->FilePath = argv[optind];
  }
}

// Constructs the command registry.
Registry BuildRegistry() {
  Registry registry;

  registry.Register('q', new ExitCommand());

  registry.Register('h', new MoveLeftCommand());
  registry.Register(KEY_LEFT, new MoveLeftCommand());
  registry.Register('j', new MoveDownCommand());
  registry.Register(KEY_DOWN, new MoveDownCommand());
  registry.Register('k', new MoveUpCommand());
  registry.Register(KEY_UP, new MoveUpCommand());
  registry.Register('l', new MoveRightCommand());
  registry.Register(KEY_RIGHT, new MoveRightCommand());
  registry.Register(' ', new ScreenDownCommand());
  registry.Register(KEY_NPAGE, new ScreenDownCommand());
  registry.Register(6, new ScreenDownCommand());  // ^F
  registry.Register(KEY_PPAGE, new ScreenUpCommand());
  registry.Register(2, new ScreenUpCommand());    // ^B

  registry.Register('=', new ZoomInCommand());
  registry.Register('+', new ZoomInCommand());
  registry.Register('-', new ZoomOutCommand());

  return registry;
}

int main(int argc, char *argv[]) {
  // Main program state.
  State state;

  // 1. Initialization.
  ParseCommandLine(argc, argv, &state);

  Document *doc = PDFDocument::Open(state.FilePath);
  if (doc == NULL) {
    fprintf(stderr, "Failed to open document \"%s\".\n",
            state.FilePath.c_str());
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
  const Registry &registry = BuildRegistry();

  initscr();
  keypad(stdscr, true);
  nonl();
  cbreak();
  noecho();

  // 2. Main event loop.
  state.Render = true;
  do {
    // 2.1 Render.
    if (state.Render) {
      viewer.SetState(state);
      viewer.Render();
      viewer.GetState(&state);
    }
    state.Render = true;

    // 2.2. Grab input.
    int repeat = Command::NO_REPEAT;
    int c;
    while (isdigit(c = getch())) {
      if (repeat == Command::NO_REPEAT) {
        repeat = c - '0';
      } else {
        repeat = repeat * 10 + c - '0';
      }
    }

    // 2.3. Run command.
    registry.Dispatch(c, repeat, &state);

  } while (!state.Exit);


  // 3. Clean up.
  endwin();

  delete fb;
  delete doc;

  return EXIT_SUCCESS;
}

