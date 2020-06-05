/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2020-2020 Chuan Ji                                         *
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

// This file declares the Settings class, which manages this app's persistent
// settings.

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>

#include "rapidjson/document.h"

class Settings {
 public:
  // Default JSON parsing flags. We want to be generally permissive with
  // settings files as they are hand edited.
  enum {
    PERMISSIVE_JSON_PARSE_FLAGS =
        rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag |
        rapidjson::kParseEscapedApostropheFlag | rapidjson::kParseNanAndInfFlag
  };

  // Factory method to create and initialize a Settings instance.
  // config_file_path and history_file_path will fall back to their respective
  // defaults if empty. Caller owns returned instance.
  static Settings* Open(
      const std::string& config_file_path,
      const std::string& history_file_path);

  // Gets the value of a string setting, with default config as fallback.
  std::string GetString(const std::string& key);
  // Gets the value of an integer setting, with default config as fallback.
  int GetInt(const std::string& key);

  // Returns the default configuration.
  static const rapidjson::Document& GetDefaultConfig();

 private:
  // Path to config file.
  std::string _config_file_path;
  // Loaded config.
  rapidjson::Document _config;

  // Path to history file.
  std::string _history_file_path;
  // Loaded history file.
  rapidjson::Document _history;

  // Use factory method Open() to create and initialize an instance.
  Settings(){};
};

#endif

