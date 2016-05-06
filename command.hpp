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

// This file declares abstractions for user commands.

#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <unordered_map>
#include <memory>
#include <string>

// Program state.
struct State;

// Interface for a user command.
class Command {
 public:
  // A constant for Execute, specifying that the user did not enter a repeat
  // number.
  static const int NO_REPEAT;
  // Executes a command. Repeat is an integer specifying how many times the
  // command should be repeated. If the user did not specify a number, NO_REPEAT
  // is passed. state is a handle to the program state (defined in main.cpp).
  virtual void Execute(int repeat, State* state) = 0;
  // A handy shortcut for (repeat == NO_REPEAT ? x : repeat).
  int RepeatOrDefault(int repeat, int default_repeat) const {
    return (repeat == NO_REPEAT) ? default_repeat : repeat;
  }
  // This is to make C++ happy.
  virtual ~Command() {}
};

// A command registry. Maintains a mapping from a key to a command pointer.
// Keeps ownership of command instances.
class Registry {
 public:
  // Releases registered commands.
  ~Registry();
  // Associates a command with a key. The key must not have been already in use.
  void Register(int key, std::unique_ptr<Command> command);
  // Executes the command associated with a key, with the given repeat argument.
  // If no command is associated with the key, returns false. Otherwise returns
  // true.
  bool Dispatch(int key, int repeat, State* state) const;

 private:
  // Maintains the mapping.
  std::unordered_map<int, std::unique_ptr<Command>> _map;
};

#endif

