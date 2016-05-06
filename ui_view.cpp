/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2014 Chuan Ji <ji@chu4n.com>                               *
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

#include "ui_view.hpp"
#include <cassert>

std::mutex UIView::_mutex;
WINDOW* UIView::_window = nullptr;
int UIView::_num_instances = 0;

UIView::UIView(const UIView::KeyProcessingModeMap& key_processing_mode_map)
    : _key_processing_mode_map(key_processing_mode_map) {
  std::unique_lock<std::mutex> lock(_mutex);

  // Initialize NCURSES if this is the first instance of UIView.
  if (_num_instances == 0) {
    _window = newwin(0, 0, 0, 0);
    keypad(_window, true);
  }

  ++_num_instances;
}

UIView::~UIView() {
  std::unique_lock<std::mutex> lock(_mutex);

  --_num_instances;
  assert(_num_instances >= 0);

  // Clean up NCURSES if this is the last instance of UIView.
  if (_num_instances == 0) {
    delwin(_window);
  }
  _window = nullptr;
}

void UIView::EventLoop(int initial_key_processing_mode) {
  _key_processing_mode = initial_key_processing_mode;
  _exit_event_loop = false;
  do {
    Render();

    const KeyProcessor& key_processor = _key_processing_mode_map.at(
        _key_processing_mode);
    key_processor(wgetch(_window));
  } while (!_exit_event_loop);
}

void UIView::ExitEventLoop() {
  _exit_event_loop = true;
}

void UIView::SwitchKeyProcessingMode(int new_key_processing_mode) {
  _key_processing_mode = new_key_processing_mode;
}

WINDOW* UIView::GetWindow() const {
  assert(_window != nullptr);
  return _window;
}
