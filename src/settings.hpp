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

#include <functional>
#include <string>
#include <unordered_map>

#include "rapidjson/document.h"

// Default config. This is generated during the build process from
// "default_config.json" by "generate_default_config_cpp.cpp".
extern const char* DEFAULT_CONFIG_JSON;

// Default per-file config. This is generated during the build process from
// "default_file_config.json" by "generate_default_config_cpp.cpp".
extern const char* DEFAULT_FILE_CONFIG_JSON;

class Settings {
 public:
  // Default JSON parsing flags. We want to be generally permissive with
  // settings files as they are hand edited.
  enum {
    PERMISSIVE_JSON_PARSE_FLAGS =
        rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag |
        rapidjson::kParseEscapedApostropheFlag |
        rapidjson::kParseNanAndInfFlag | rapidjson::kParseStopWhenDoneFlag
  };

  // Factory method to create and initialize a Settings instance.
  // config_file_path and history_file_path will fall back to their respective
  // defaults if empty. Caller owns returned instance.
  static Settings* Open(
      const std::string& config_file_path,
      const std::string& history_file_path);

  // Gets the value of a string setting, with default config as fallback.
  std::string GetStringSetting(const std::string& key) const;
  // Gets the value of a string setting from 1) history, 2) user config, or 3)
  // default config in that order.
  std::string GetStringSettingForFile(
      const std::string& file_path, const std::string& key) const;
  // Gets the value of an integer setting, with default config as fallback.
  int GetIntSetting(const std::string& key) const;
  // Gets the value of an integer setting from 1) history, 2) user config, or 3)
  // default config in that order.
  int GetIntSettingForFile(
      const std::string& file_path, const std::string& key) const;
  // Gets the value of an enum setting, with default config as fallback.
  // Possible enum options are specified as a STL map or unordered_map from
  // string to value.
  template <
      typename V = int, typename MapT = std::unordered_map<std::string, V> >
  V GetEnumSetting(const std::string& key, const MapT& enum_map) const;
  // Gets the value of an enum setting from 1) history, 2) user config, or 3)
  // default config in that order.
  template <
      typename V = int, typename MapT = std::unordered_map<std::string, V> >
  V GetEnumSettingForFile(
      const std::string& file_path, const std::string& key,
      const MapT& enum_map) const;

  // Returns the default configuration.
  static const rapidjson::Document& GetDefaultConfig();
  // Returns the default per-file configuration.
  static const rapidjson::Document& GetDefaultFileConfig();

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

  // Type alias for a function that checks whether a setting value is valid.
  template <typename T>
  using ConfigValueValidationFn = std::function<bool(const T&)>;

  // Looks up a setting from each of the provided configs in order, falling
  // through to the next config if the setting could not be found in the config
  // or if the setting fails validation (wrong type or validation function
  // provided and returns false). Pass nullptr to validation_fn if no
  // validation needed beyond type check.
  template <
      typename T, typename ConfigT = rapidjson::Value, typename... ConfigTs>
  static T GetConfigValue(
      const std::string& key, ConfigValueValidationFn<T> validation_fn,
      const ConfigT& config, const ConfigTs&... rest);
  // Base case of above variadic function that just crashes immediately.
  template <typename T>
  static T GetConfigValue(
      const std::string& key, ConfigValueValidationFn<T> validation_fn);

  // Locates per-file settings in a history JSON document, or null.
  const rapidjson::Value& GetSettingsForFile(
      const std::string& file_path) const;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                              Implementation                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

template <typename T>
T Settings::GetConfigValue(
    const std::string& key,
    Settings::ConfigValueValidationFn<T> validation_fn) {
  fprintf(stderr, "Unable to locate config value \'%s\'\n", key.c_str());
  abort();
}

template <typename T, typename ConfigT, typename... ConfigTs>
T Settings::GetConfigValue(
    const std::string& key, ConfigValueValidationFn<T> validation_fn,
    const ConfigT& config, const ConfigTs&... rest) {
  if (config.IsObject() && config.HasMember(key.c_str())) {
    const auto& value = config[key.c_str()];
    if (value.template Is<T>() &&
        (!validation_fn || validation_fn(value.template Get<T>()))) {
      return value.template Get<T>();
    }
  }
  return GetConfigValue<T, ConfigTs...>(key, validation_fn, rest...);
}

template <typename V, typename MapT>
V Settings::GetEnumSetting(const std::string& key, const MapT& enum_map) const {
  const ConfigValueValidationFn<const char*> enum_value_validation_fn =
      [&enum_map](const char* value) { return enum_map.count(value); };
  const std::string string_value = GetConfigValue<const char*>(
      key, enum_value_validation_fn, _config, GetDefaultConfig());
  typename MapT::const_iterator it = enum_map.find(string_value);
  assert(it != enum_map.end());
  return it->second;
}

template <typename V, typename MapT>
V Settings::GetEnumSettingForFile(
    const std::string& file_path, const std::string& key,
    const MapT& enum_map) const {
  const ConfigValueValidationFn<const char*> enum_value_validation_fn =
      [&enum_map](const char* value) { return enum_map.count(value); };
  const std::string string_value = GetConfigValue<const char*>(
      key, enum_value_validation_fn, GetSettingsForFile(file_path),
      GetDefaultFileConfig(), _config, GetDefaultConfig());
  typename MapT::const_iterator it = enum_map.find(string_value);
  assert(it != enum_map.end());
  return it->second;
}

#endif

