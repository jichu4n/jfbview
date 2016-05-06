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

// This file defines abstractions for user commands.

#include "command.hpp"
#include <cassert>

const int Command::NO_REPEAT = -1;

Registry::~Registry() {}

void Registry::Register(int key, std::unique_ptr<Command> command) {
  assert(!_map.count(key));
  _map.emplace(key, std::move(command));
}

bool Registry::Dispatch(int key, int repeat, State* state) const {
  const auto i = _map.find(key);
  if (i == _map.end()) {
    return false;
  }
  i->second->Execute(repeat, state);
  return true;
}

