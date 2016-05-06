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

// This file defines the UIView class, a base class for NCURSES-based
// interactive full-screen UIs.

#ifndef UI_VIEW_HPP
#define UI_VIEW_HPP

#include <curses.h>
#include <functional>
#include <mutex>
#include <unordered_map>

// Base class for NCURSES-based interactive full-screen UIs. Implements
//   - Static NCURSES window initialization on first construction of any derived
//     instances;
//   - Static NCURSES window clean up upon destruction on last destruction of
//     any derived instances;
//   - Main event loop.
class UIView {
 public:
  // A function that handles key events.
  typedef std::function<void(int)> KeyProcessor;
  // A map that maps key processing modes (as ints) to key processing methods.
  typedef std::unordered_map<int, KeyProcessor> KeyProcessingModeMap;

  // Statically initializes an NCURSES window on first construction of any
  // derived instances.
  explicit UIView(const KeyProcessingModeMap& key_processing_mode_map);

  // Statically cleans up an NCURSES window upon last destruction of any derived
  // instances.
  virtual ~UIView();

 protected:
  // Renders the current UI. This should be implemented by derived classes.
  virtual void Render() = 0;

  // Starts the event loop. This will repeatedly call fetch the next keyboard
  // event, invoke ProcessKey(), and Render(). Will exit the loop when
  // ExitEventLoop() is invoked. initial_mode specifies the initial key
  // processing mode.
  void EventLoop(int initial_key_processing_mode);
  // Causes the event loop to exit.
  void ExitEventLoop();
  // Switches key processing mode.
  void SwitchKeyProcessingMode(int new_key_processing_mode);
  // Returns the current key processing mode.
  int GetKeyProcessingMode() const { return _key_processing_mode; }

  // Returns the full-screen NCURSES window.
  WINDOW* GetWindow() const;

 private:
  // A mutex protecting static members.
  static std::mutex _mutex;
  // Reference counter. This is the number of derived instances currently
  // instantiated. Protected by _mutex.
  static int _num_instances;
  // The NCURSES WINDOW. Will be nullptr if uninitialized. Protected by _mutex.
  static WINDOW* _window;
  // Maps key processing modes to the actual handler.
  const KeyProcessingModeMap _key_processing_mode_map;
  // The current key processing mode.
  int _key_processing_mode;
  // Whether to exit the event loop.
  bool _exit_event_loop;
};

#endif
