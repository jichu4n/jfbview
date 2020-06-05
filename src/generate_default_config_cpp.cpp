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

// This script reads the contents of default_config.json from stdin, and prints
// to stdout a C++ file that exports the contents as a constant
// `DEFAULT_CONFIG_JSON'.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "settings.hpp"

int main() {
  // 1. Read stdin into a string.
  std::string stdin_contents;
  {
    while (!feof(stdin)) {
      char read_buffer[1024] = {0};
      fgets(read_buffer, sizeof(read_buffer), stdin);
      stdin_contents += read_buffer;
    }
  }

  // 2. Validate JSON.
  rapidjson::Document doc;
  {
    const rapidjson::ParseResult parse_result =
        doc.Parse<Settings::PERMISSIVE_JSON_PARSE_FLAGS>(
            stdin_contents.c_str());
    if (!parse_result) {
      fprintf(
          stderr, "Failed to parse input at position %lu: %s\n",
          parse_result.Offset(),
          rapidjson::GetParseError_En(parse_result.Code()));
      return EXIT_FAILURE;
    }
  }

  // 3. Serialize read input as string.
  std::string quoted_serialized_doc;
  {
    rapidjson::StringBuffer output_buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(output_buffer);
    writer.String(stdin_contents.c_str());
    quoted_serialized_doc = output_buffer.GetString();
  }

  // 4. Print result to stdout.
  fprintf(
      stdout, "const char* DEFAULT_CONFIG_JSON = %s;",
      quoted_serialized_doc.c_str());

  return EXIT_SUCCESS;
}

