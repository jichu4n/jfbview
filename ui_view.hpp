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

// This file defines the UIView class, a base class for NCURSES-based
// interactive full-screen UIs.

#ifndef UI_VIEW_HPP
#define UI_VIEW_HPP

#include <curses.h>
#include <mutex>

// Base class for NCURSES-based interactive full-screen UIs. Implements
//   - Static NCURSES initialization on first construction of any derived
//     instances;
//   - Static NCURSES clean up upon destruction on last destruction of any
//     derived instances;
//   - Main event loop.
class UIView {
 public:
  // Statically initializes NCURSES on first construction of any derived
  // instances.
  UIView();
  // Statically cleans up NCURSES upon last destruction of any derived
  // instances.
  virtual ~UIView();

 protected:
  // Renders the current UI. This should be implemented by derived classes.
  virtual void Render() = 0;
  // This will be invoked to process a keyboard event during EventLoop().
  virtual void ProcessKey(int key) = 0;

  // Starts the event loop. This will repeatedly call fetch the next keyboard
  // event, invoke ProcessKey(), and Render(). Will exit the loop when
  // ExitEventLoop() is invoked.
  void EventLoop();
  // Causes the event loop to exit.
  void ExitEventLoop();

  // Returns the full-screen NCURSES window.
  WINDOW* GetWindow() const { return _window; }

 private:
  // A mutex protecting static members.
  static std::mutex _mutex;
  // Reference counter. This is the number of derived instances currently
  // instantiated. Protected by _mutex.
  static int _num_instances;
  // The NCURSES WINDOW. Will be nullptr if uninitialized. Protected by _mutex.
  static WINDOW* _window;
  // Whether to exit the event loop.
  bool _exit_event_loop;
};

#endif
