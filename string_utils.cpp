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

#include "string_utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <string>

namespace {

// Returns !isspace(c).
const std::function<int(int)> NotIsSpaceFn =
    std::not1(std::ptr_fun<int, int>(std::isspace));

}  // namespace

std::string TrimLeft(const std::string& s) {
  std::string r(s);
  r.erase(r.begin(), std::find_if(r.begin(), r.end(), NotIsSpaceFn));
  return r;
}

std::string TrimRight(const std::string& s) {
  std::string r(s);
  r.erase(std::find_if(r.rbegin(), r.rend(), NotIsSpaceFn).base(), r.end());
  return r;
}

std::string Trim(const std::string& s) {
  return TrimLeft(TrimRight(s));
}

std::string::size_type CaseInsensitiveSearch(
    const std::string& s,
    const std::string& search_string,
    std::string::size_type pos) {
  if (pos == std::string::npos || pos > s.length()) {
    return std::string::npos;
  }
  const char* const p = strcasestr(s.c_str() + pos, search_string.c_str());
  if (p == nullptr) {
    return std::string::npos;
  }
  return p - s.c_str();
}

