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

// Program name. May be overridden in the Makefile.
#ifndef JFBVIEW_PROGRAM_NAME
#define JFBVIEW_PROGRAM_NAME "JFBView"
#endif

// Binary program name. May be overridden in the Makefile.
#ifndef JFBVIEW_BINARY_NAME
#define JFBVIEW_BINARY_NAME "jfbview"
#endif

#include <curses.h>
#include <getopt.h>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include "command.hpp"
#include "framebuffer.hpp"
#include "image_document.hpp"
#include "outline_viewer.hpp"
#include "pdf_document.hpp"
#include "viewer.hpp"

// Main program state.
struct State: public Viewer::State {
  // If true, exit main event loop.
  bool Exit;
  // If true (default), requires refresh after current command.
  bool Render;

  // The type of the displayed file.
  enum {
    AUTO_DETECT,
    PDF,
#ifndef JFBVIEW_NO_IMLIB2
    IMAGE,
#endif
  } DocumentType;
  // Viewer render cache size.
  int RenderCacheSize;
  // Input file.
  std::string FilePath;
  // Framebuffer device.
  std::string FramebufferDevice;
  // Document instance.
  std::unique_ptr<Document> DocumentInst;
  // Outline viewer instance.
  std::unique_ptr<OutlineViewer> OutlineViewerInst;
  // Framebuffer instance.
  std::unique_ptr<Framebuffer> FramebufferInst;
  // Viewer instance.
  std::unique_ptr<Viewer> ViewerInst;

  // Default state.
  State()
      : Viewer::State(), Exit(false), Render(true), DocumentType(AUTO_DETECT),
        RenderCacheSize(Viewer::DEFAULT_RENDER_CACHE_SIZE), FilePath(""),
        FramebufferDevice(Framebuffer::DEFAULT_FRAMEBUFFER_DEVICE),
        OutlineViewerInst(nullptr), FramebufferInst(nullptr),
        ViewerInst(nullptr) {
  }
};

// Returns the all lowercase version of a string.
static std::string ToLower(const std::string &s) {
  std::string r(s);
  std::transform(r.begin(), r.end(), r.begin(), &tolower);
  return r;
}

// Returns the file extension of a path, or the empty string. The extension is
// converted to lower case.
static std::string GetFileExtension(const std::string &path) {
  int path_len = path.length();
  if ((path_len >= 4) && (path[path_len - 4] == '.')) {
    return ToLower(path.substr(path_len - 3));
  }
  return std::string();
}

// Loads the file specified in a state. Returns true if the file has been
// loaded.
static bool LoadFile(State *state) {
  if (state->DocumentType == State::AUTO_DETECT) {
    if (GetFileExtension(state->FilePath) == "pdf") {
      state->DocumentType = State::PDF;
    } else {
#ifndef JFBVIEW_NO_IMLIB2
      state->DocumentType = State::IMAGE;
#else
      fprintf(stderr, "Cannot detect file format. Plase specify a file format "
                      "with --format. Try --help for help.\n");
      return false;
#endif
    }
  }
  Document *doc = nullptr;
  switch (state->DocumentType) {
    case State::PDF:
      doc = PDFDocument::Open(state->FilePath);
      break;
#ifndef JFBVIEW_NO_IMLIB2
    case State::IMAGE:
      doc = ImageDocument::Open(state->FilePath);
      break;
#endif
    default:
      abort();
  }
  if (doc == nullptr) {
    fprintf(stderr, "Failed to open document \"%s\".\n",
            state->FilePath.c_str());
    return false;
  }
  state->DocumentInst.reset(doc);
  return true;
}


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
      if (++(state->Page) < state->NumPages) {
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
      if (++(state->Page) < state->NumPages) {
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

class PageDownCommand: public Command {
 public:
  virtual void Execute(int repeat, State *state) {
    state->Page += RepeatOrDefault(repeat, 1);
  }
};

class PageUpCommand: public Command {
 public:
  virtual void Execute(int repeat, State *state) {
    state->Page -= RepeatOrDefault(repeat, 1);
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
    const float new_page_width =
        static_cast<float>(state->PageWidth) * q;
    const float new_page_height =
        static_cast<float>(state->PageHeight) * q;
    // New center position within page after zoom change.
    const float new_center_x = new_page_width * center_ratio_x;
    const float new_center_y = new_page_height * center_ratio_y;
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

class SetZoomCommand: public ZoomCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    SetZoom(static_cast<float>(RepeatOrDefault(repeat, 100)) / 100.0f,
            state);
  }
};

class SetRotationCommand: public Command {
 public:
  virtual void Execute(int repeat, State *state) {
    state->Rotation = RepeatOrDefault(repeat, 0);
  }
};

class RotateCommand: public Command {
 public:
  explicit RotateCommand(int increment)
      : _increment(increment) {
  }

  virtual void Execute(int repeat, State *state) {
    state->Rotation += RepeatOrDefault(repeat, 1) * _increment;
  }
 private:
  int _increment;
};

class ZoomToFitCommand: public Command {
 public:
  virtual void Execute(int repeat, State *state) {
    state->Zoom = Viewer::ZOOM_TO_FIT;
  }
};

class ZoomToWidthCommand: public ZoomCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    // Estimate page width at 100%.
    const float orig_page_width =
        static_cast<float>(state->PageWidth) / state->ActualZoom;
    // Estimate actual zoom ratio with zoom to width.
    const float actual_zoom =
        static_cast<float>(state->ScreenWidth) / orig_page_width;
    // Set center according to estimated actual zoom.
    SetZoom(actual_zoom, state);
    // Actually set zoom to width.
    state->Zoom = Viewer::ZOOM_TO_WIDTH;
  }
};

class GoToPageCommand: public Command {
 public:
  explicit GoToPageCommand(int default_page)
      : _default_page(default_page) {}
  virtual void Execute(int repeat, State *state) {
    int page = (std::max(1, std::min(state->NumPages,
        RepeatOrDefault(repeat, _default_page)))) - 1;
    if (page != state->Page) {
      state->Page = page;
      state->XOffset = 0;
      state->YOffset = 0;
    }
  }
 private:
  int _default_page;
};

class ShowOutlineViewerCommand: public Command {
 public:
  virtual void Execute(int repeat, State *state) {
    const Document::OutlineItem *dest = state->OutlineViewerInst->Show();
    if (dest != nullptr) {
      int dest_page = state->DocumentInst->Lookup(dest);
      if (dest_page >= 0) {
        GoToPageCommand c(0);
        c.Execute(dest_page + 1, state);
      }
    }
  }
};

// Base class for SaveStateCommand and RestoreStateCommand.
class StateCommand: public Command {
 protected:
  // A global map from register number to saved state.
  static std::map<int, Viewer::State> _saved_states;
};
std::map<int, Viewer::State> StateCommand::_saved_states;

class SaveStateCommand: public StateCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    state->ViewerInst->GetState(
        &(_saved_states[RepeatOrDefault(repeat, 0)]));
    state->Render = false;
  }
};

class RestoreStateCommand: public StateCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    int n = RepeatOrDefault(repeat, 0);
    if (_saved_states.count(n)) {
      state->ViewerInst->SetState(_saved_states[n]);
      state->ViewerInst->GetState(state);
    }
  }
};

class ReloadCommand: public StateCommand {
 public:
  virtual void Execute(int repeat, State *state) {
    if (LoadFile(state)) {
      state->ViewerInst.reset(new Viewer(
        state->DocumentInst.get(), state->FramebufferInst.get(), *state,
        state->RenderCacheSize));
    } else {
      state->Exit = true;
    }
  }
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                               END COMMANDS                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


// Help text printed by --help or -h.
static const char *HELP_STRING =
    JFBVIEW_PROGRAM_NAME " v0.2\n"
    "Copyright (C) 2012 Chuan Ji <jichuan89@gmail.com>\n"
    "\n"
    "Licensed under the Apache License, Version 2.0 (the \"License\");\n"
    "you may not use this file except in compliance with the License.\n"
    "You may obtain a copy of the License at\n"
    "\n"
    "    http://www.apache.org/licenses/LICENSE-2.0\n"
    "\n"
    "Unless required by applicable law or agreed to in writing, software\n"
    "distributed under the License is distributed on an \"AS IS\" BASIS,\n"
    "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
    "See the License for the specific language governing permissions and\n"
    "limitations under the License.\n"
    "\n"
    "Usage: " JFBVIEW_BINARY_NAME " [OPTIONS] FILE\n"
    "\n"
    "Options:\n"
    "\t--help, -h            Show this message.\n"
    "\t--fb=/path/to/dev     Specify output framebuffer device.\n"
    "\t--page=N, -p N        Open page N on start up.\n"
    "\t--zoom=N, -z N        Set initial zoom to N. E.g., -z 150 sets \n"
    "\t                      zoom level to 150%.\n"
    "\t--zoom_to_fit         Start in automatic zoom-to-fit mode.\n"
    "\t--zoom_to_width       Start in automatic zoom-to-width mode.\n"
    "\t--rotation=N, -r N    Set initial rotation to N degrees clockwise.\n"
#ifndef JFBVIEW_NO_IMLIB2
    "\t--format=image, -f image\n"
    "\t                      Forces the program to treat the input file as an\n"
    "\t                      image.\n"
#endif
    "\t--format=pdf, -f pdf  Forces the program to treat the input file as a\n"
    "\t                      PDF document. Use this if your PDF file does not\n"
    "\t                      end in \".pdf\" (case is ignored).\n"
    "\t--cache_size=N        Cache at most N pages. If you have an older\n"
    "\t                      machine with limited RAM, or if you are loading\n"
    "\t                      huge documents, or if you just want to reduce\n"
    "\t                      memory usage, you might want to set this to a\n"
    "\t                      smaller number.\n";

// Parses the command line, and stores settings in state. Crashes the program if
// the commnad line contains errors.
void ParseCommandLine(int argc, char *argv[], State *state) {
  // Tags for long options that don't have short option chars.
  enum {
    RENDER_CACHE_SIZE = 0x1000,
    ZOOM_TO_WIDTH,
    ZOOM_TO_FIT,
    FB,
  };
  // Command line options.
  static const option LongFlags[] = {
      { "help", false, nullptr, 'h' },
      { "fb", true, nullptr, FB },
      { "page", true, nullptr, 'p' },
      { "zoom", true, nullptr, 'z' },
      { "zoom_to_width", false, nullptr, ZOOM_TO_WIDTH },
      { "zoom_to_fit", false, nullptr, ZOOM_TO_FIT },
      { "rotation", true, nullptr, 'r' },
      { "cache_size", true, nullptr, RENDER_CACHE_SIZE },
      { "format", true, nullptr, 'f' },
      { 0, 0, 0, 0 },
  };
  static const char *ShortFlags = "hp:z:r:f:";

  for (;;) {
    int opt_char = getopt_long(argc, argv, ShortFlags, LongFlags, nullptr);
    if (opt_char == -1) {
      break;
    }
    switch (opt_char) {
     case 'h':
       fprintf(stdout, "%s", HELP_STRING);
       exit(EXIT_FAILURE);
     case FB:
       state->FramebufferDevice = optarg;
       break;
     case 'f':
       if (ToLower(optarg) == "pdf") {
         state->DocumentType = State::PDF;
#ifndef JFBVIEW_NO_IMLIB2
       } else if (ToLower(optarg) == "image") {
         state->DocumentType = State::IMAGE;
#endif
       } else {
        fprintf(stderr, "Invalid file format \"%s\"\n", optarg);
        exit(EXIT_FAILURE);
       }
       break;
     case RENDER_CACHE_SIZE:
      if (sscanf(optarg, "%d", &(state->RenderCacheSize)) < 1) {
        fprintf(stderr, "Invalid render cache size \"%s\"\n", optarg);
        exit(EXIT_FAILURE);
      }
      state->RenderCacheSize = std::max(1, state->RenderCacheSize + 1);
      break;
     case 'p':
      if (sscanf(optarg, "%d", &(state->Page)) < 1) {
        fprintf(stderr, "Invalid page number \"%s\"\n", optarg);
        exit(EXIT_FAILURE);
      }
      --(state->Page);
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
      fprintf(stderr, "Try \"-h\" for help.\n");
      exit(EXIT_FAILURE);
    }
  }
  if (optind == argc) {
    fprintf(stderr, "No file specified. Try \"-h\" for help.\n");
    exit(EXIT_FAILURE);
  } else if (optind < argc - 1) {
    fprintf(stderr, "Please specify exactly one input file. Try \"-h\" for "
                    "help.\n");
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
  registry.Register(6, new ScreenDownCommand());  // ^F
  registry.Register(2, new ScreenUpCommand());    // ^B
  registry.Register('J', new PageDownCommand());
  registry.Register(KEY_NPAGE, new PageDownCommand());
  registry.Register('K', new PageUpCommand());
  registry.Register(KEY_PPAGE, new PageUpCommand());

  registry.Register('=', new ZoomInCommand());
  registry.Register('+', new ZoomInCommand());
  registry.Register('-', new ZoomOutCommand());
  registry.Register('z', new SetZoomCommand());
  registry.Register('s', new ZoomToWidthCommand());
  registry.Register('a', new ZoomToFitCommand());

  registry.Register('r', new SetRotationCommand());
  registry.Register('>', new RotateCommand(90));
  registry.Register('.', new RotateCommand(90));
  registry.Register('<', new RotateCommand(-90));
  registry.Register(',', new RotateCommand(-90));

  registry.Register('g', new GoToPageCommand(0));
  registry.Register(KEY_HOME, new GoToPageCommand(0));
  registry.Register('G', new GoToPageCommand(INT_MAX));
  registry.Register(KEY_END, new GoToPageCommand(INT_MAX));

  registry.Register('\t', new ShowOutlineViewerCommand());

  registry.Register('m', new SaveStateCommand());
  registry.Register('`', new RestoreStateCommand());

  registry.Register('e', new ReloadCommand());

  return registry;
}

int main(int argc, char *argv[]) {
  // Main program state.
  State state;

  // 1. Initialization.
  ParseCommandLine(argc, argv, &state);

  if (!LoadFile(&state)) {
    exit(EXIT_FAILURE);
  }
  state.FramebufferInst.reset(Framebuffer::Open(state.FramebufferDevice));
  if (state.FramebufferInst == nullptr) {
    fprintf(stderr, "Failed to initialize framebuffer device \"%s\".\n",
            state.FramebufferDevice.c_str());
    exit(EXIT_FAILURE);
  }
  state.ViewerInst.reset(new Viewer(
      state.DocumentInst.get(), state.FramebufferInst.get(), state,
      state.RenderCacheSize));
  const Registry &registry = BuildRegistry();

  initscr();
  keypad(stdscr, true);
  nonl();
  cbreak();
  noecho();
  curs_set(false);
  // This is necessary to prevent curses erasing the framebuffer on first call
  // to getch().
  refresh();

  state.OutlineViewerInst.reset(
      new OutlineViewer(state.DocumentInst->GetOutline()));

  // 2. Main event loop.
  state.Render = true;
  do {
    // 2.1 Render.
    if (state.Render) {
      state.ViewerInst->SetState(state);
      state.ViewerInst->Render();
      state.ViewerInst->GetState(&state);
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
  state.OutlineViewerInst.reset();
  endwin();

  return EXIT_SUCCESS;
}

