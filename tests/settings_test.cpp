#include "../src/settings.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <memory>
#include <string>

#include "../src/viewer.hpp"
#include "rapidjson/prettywriter.h"
#include "rapidjson/rapidjson.h"

class TempFile {
 public:
  const std::string FilePath;
  FILE* const FilePtr;

  static TempFile* Create() {
    char file_path_buffer[] = "/tmp/jfbview-test.XXXXXX";
    int fd = mkstemp(file_path_buffer);
    EXPECT_GT(fd, 1);
    return new TempFile(file_path_buffer, fdopen(fd, "r+"));
  }

  ~TempFile() {
    if (FilePtr != nullptr) {
      EXPECT_EQ(fclose(FilePtr), 0);
    }
    EXPECT_EQ(unlink(FilePath.c_str()), 0);
  }

 private:
  TempFile(const std::string& file_path, FILE* file_ptr)
      : FilePath(file_path), FilePtr(file_ptr) {}
  TempFile(const TempFile& other);
};

class SettingsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    _config_file.reset(TempFile::Create());
    _history_file.reset(TempFile::Create());
    ReloadSettings();
  }

  void TearDown() override {
    _settings.reset();
    _history_file.reset();
    _config_file.reset();
  }

  void ReloadSettings() {
    EXPECT_EQ(fflush(_config_file->FilePtr), 0);
    EXPECT_EQ(fflush(_history_file->FilePtr), 0);
    _settings.reset(
        Settings::Open(_config_file->FilePath, _history_file->FilePath));
  }

  std::unique_ptr<TempFile> _config_file;
  std::unique_ptr<TempFile> _history_file;
  std::unique_ptr<Settings> _settings;
};

TEST_F(SettingsTest, CanLoadDefaultSettings) {
  fprintf(stderr, "Default settings:\n%s", DEFAULT_CONFIG_JSON);
  const rapidjson::Document& default_config = Settings::GetDefaultConfig();
  rapidjson::StringBuffer output_buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output_buffer);
  default_config.Accept(writer);
  const std::string output = output_buffer.GetString();
  EXPECT_GT(output.length(), 2);
  fprintf(stderr, "Loaded default settings:\n%s\n", output.c_str());
}

TEST_F(SettingsTest, GetValuesWithEmptyConfig) {
  const std::string fb = _settings->GetStringSetting("fb");
  EXPECT_GT(fb.length(), 0);
  EXPECT_EQ(fb, Settings::GetDefaultConfig()["fb"].GetString());

  int cache_size = _settings->GetIntSetting("cacheSize");
  EXPECT_GT(cache_size, 0);
  EXPECT_EQ(cache_size, Settings::GetDefaultConfig()["cacheSize"].GetInt());

  float zoom_mode = _settings->GetEnumSetting<float>(
      "zoomMode", Viewer::ZOOM_MODE_ENUM_OPTIONS);
  EXPECT_NE(zoom_mode, 0.0f);

  int color_mode = -1;
  color_mode = _settings->GetEnumSetting<Viewer::ColorMode>(
      "colorMode", Viewer::COLOR_MODE_ENUM_OPTIONS);
  EXPECT_GE(color_mode, 0);
}

TEST_F(SettingsTest, GetValuesWithCustomConfig) {
  const char* custom_fb_value = "/dev/foobar";
  const int custom_cache_size = 42;
  const char* custom_zoom_mode = "original";
  const char* custom_color_mode = "sepia";
  fprintf(
      _config_file->FilePtr,
      "{"
      "\"fb\": \"%s\","
      "\"cacheSize\": %d,"
      "\"zoomMode\": \"%s\","
      "\"colorMode\": \"%s\","
      "}",
      custom_fb_value, custom_cache_size, custom_zoom_mode, custom_color_mode);

  ReloadSettings();

  const std::string fb = _settings->GetStringSetting("fb");
  EXPECT_EQ(fb, custom_fb_value);

  const int cache_size = _settings->GetIntSetting("cacheSize");
  EXPECT_EQ(cache_size, custom_cache_size);

  float zoom_mode = _settings->GetEnumSetting<float>(
      "zoomMode", Viewer::ZOOM_MODE_ENUM_OPTIONS);
  EXPECT_EQ(zoom_mode, Viewer::ZOOM_MODE_ENUM_OPTIONS.at(custom_zoom_mode));

  int color_mode = -1;
  color_mode = _settings->GetEnumSetting<Viewer::ColorMode>(
      "colorMode", Viewer::COLOR_MODE_ENUM_OPTIONS);
  EXPECT_EQ(color_mode, Viewer::COLOR_MODE_ENUM_OPTIONS.at(custom_color_mode));
}

TEST_F(SettingsTest, GetEnumWithInvalidCustomConfig) {
  const rapidjson::Document& default_config = Settings::GetDefaultConfig();
  fprintf(
      _config_file->FilePtr,
      "{"
      "\"zoomMode\": \"\","
      "\"colorMode\": \"asdf\","
      "}");

  ReloadSettings();

  float zoom_mode = _settings->GetEnumSetting<float>(
      "zoomMode", Viewer::ZOOM_MODE_ENUM_OPTIONS);
  EXPECT_EQ(
      zoom_mode, Viewer::ZOOM_MODE_ENUM_OPTIONS.at(
                     default_config["zoomMode"].GetString()));

  int color_mode = -1;
  color_mode = _settings->GetEnumSetting<Viewer::ColorMode>(
      "colorMode", Viewer::COLOR_MODE_ENUM_OPTIONS);
  EXPECT_EQ(
      color_mode, Viewer::COLOR_MODE_ENUM_OPTIONS.at(
                      default_config["colorMode"].GetString()));
}

