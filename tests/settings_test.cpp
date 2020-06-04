#include "../src/settings.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <memory>

#include "rapidjson/prettywriter.h"
#include "rapidjson/rapidjson.h"

TEST(Settings, CanLoadDefaultSettings) {
  const rapidjson::Document& default_config = Settings::GetDefaultConfig();
  rapidjson::StringBuffer output_buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output_buffer);
  default_config.Accept(writer);
  const std::string output = output_buffer.GetString();
  EXPECT_GT(output.length(), 2);
  fprintf(stderr, "Loaded default settings:\n%s\n", output.c_str());
}

