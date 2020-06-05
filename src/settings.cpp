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

#include "settings.hpp"

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>

#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/rapidjson.h"

namespace {

// Default file name of the config file.
const char* DEFAULT_CONFIG_FILE_NAME = "config.json";
// Default file name of the history file.
const char* DEFAULT_HISTORY_FILE_NAME = "history.json";

const size_t IO_BUFFER_SIZE = 64 * 1024;

// Returns the user's home directory path.
std::string GetHomeDirPath() {
  const char* home_env_var = getenv("HOME");
  if (home_env_var != nullptr && strnlen(home_env_var, 1)) {
    return home_env_var;
  }
  const passwd* passwd_entry = getpwuid(getuid());
  if (passwd_entry != nullptr) {
    return passwd_entry->pw_dir;
  }
  return "";
}

// Returns our config directory path.
std::string GetConfigDirPath() {
  std::string config_root_dir_path;
  const char* xdg_config_home_env_var = getenv("XDG_CONFIG_HOME");
  if (xdg_config_home_env_var != nullptr &&
      strnlen(xdg_config_home_env_var, 1)) {
    config_root_dir_path = xdg_config_home_env_var;
  } else {
    const std::string home_dir_path = GetHomeDirPath();
    if (home_dir_path.empty()) {
      return "";
    }
    config_root_dir_path = home_dir_path + "/.config";
  }
  return config_root_dir_path + "/jfbview";
}

// Returns the default location of our config file.
std::string GetDefaultConfigFilePath() {
  const std::string config_root_dir_path = GetConfigDirPath();
  if (config_root_dir_path.empty()) {
    return "";
  }
  return config_root_dir_path + "/" + DEFAULT_CONFIG_FILE_NAME;
}

// Returns the default location of our history file.
std::string GetDefaultHistoryFilePath() {
  const std::string config_root_dir_path = GetConfigDirPath();
  if (config_root_dir_path.empty()) {
    return "";
  }
  return config_root_dir_path + "/" + DEFAULT_HISTORY_FILE_NAME;
}

void LoadJsonFromFile(const std::string& file_path, rapidjson::Document* doc) {
  FILE* file;
  if (file_path.length() && (file = fopen(file_path.c_str(), "r")) != nullptr) {
    char buffer[IO_BUFFER_SIZE];
    rapidjson::FileReadStream read_stream(file, buffer, sizeof(buffer));
    doc->ParseStream<Settings::PERMISSIVE_JSON_PARSE_FLAGS>(read_stream);
    fclose(file);
  }
}

void WriteJsonToFile(
    const rapidjson::Document& doc, const std::string& file_path) {
  FILE* file;
  if (file_path.empty() || (file = fopen(file_path.c_str(), "w")) == nullptr) {
    return;
  }
  char buffer[IO_BUFFER_SIZE];
  rapidjson::FileWriteStream write_stream(file, buffer, sizeof(buffer));
  rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(write_stream);
  doc.Accept(writer);
  fclose(file);
}

// Parsed default config.
std::unique_ptr<rapidjson::Document> DefaultConfig;
// Mutex guarding default config.
std::mutex DefaultConfigMutex;
// Parse DEFAULT_CONFIG_JSON into DefaultConfig if not already parsed.
void ParseDefaultConfig() {
  std::lock_guard<std::mutex> lock(DefaultConfigMutex);
  if (DefaultConfig != nullptr) {
    return;
  }
  std::unique_ptr<rapidjson::Document> doc =
      std::make_unique<rapidjson::Document>();
  const rapidjson::ParseResult parse_result =
      doc->Parse<Settings::PERMISSIVE_JSON_PARSE_FLAGS>(DEFAULT_CONFIG_JSON);
  if (!parse_result) {
    fprintf(
        stderr, "Failed to parse default config at position %lu: %s\n",
        parse_result.Offset(),
        rapidjson::GetParseError_En(parse_result.Code()));
    abort();
  }
  assert(doc->IsObject());
  DefaultConfig = std::move(doc);
}

// Returns a value from the default config. Fails if the key could not be found
// or is of the wrong type.
template <typename T>
T GetDefaultConfigValue(const std::string& key) {
  ParseDefaultConfig();
  assert(DefaultConfig.get() != nullptr);
  if (DefaultConfig->HasMember(key.c_str()) &&
      (*DefaultConfig)[key.c_str()].Is<T>()) {
    return (*DefaultConfig)[key.c_str()].Get<T>();
  }
  fprintf(
      stderr, "Unable to locate default config value \'%s\'\n", key.c_str());
  abort();
}

// Returns a value from the provided config or fall back to the default config.
template <typename T>
T GetConfigValueOrDefault(
    const rapidjson::Value& config, const std::string& key) {
  if (config.IsObject() && config.HasMember(key.c_str()) &&
      config[key.c_str()].Is<T>()) {
    return config[key.c_str()].Get<T>();
  }
  return GetDefaultConfigValue<T>(key);
}

}  // namespace

Settings* Settings::Open(
    const std::string& config_file_path, const std::string& history_file_path) {
  ParseDefaultConfig();

  Settings* settings = new Settings();
  settings->_config_file_path =
      config_file_path.length() ? config_file_path : GetDefaultConfigFilePath();
  settings->_history_file_path = history_file_path.length()
                                     ? history_file_path
                                     : GetDefaultHistoryFilePath();
  LoadJsonFromFile(settings->_config_file_path, &settings->_config);
  LoadJsonFromFile(settings->_history_file_path, &settings->_history);
  return settings;
}

std::string Settings::GetString(const std::string& key) {
  return GetConfigValueOrDefault<const char*>(_config, key);
}

int Settings::GetInt(const std::string& key) {
  return GetConfigValueOrDefault<int>(_config, key);
}

const rapidjson::Document& Settings::GetDefaultConfig() {
  ParseDefaultConfig();
  assert(DefaultConfig.get() != nullptr);
  return *DefaultConfig;
}

