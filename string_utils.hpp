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

// A collection of string manipulation utilities.

#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>

// Trim leading whitespace.
extern std::string TrimLeft(const std::string& s);
// Trim trailing whitespace.
extern std::string TrimRight(const std::string& s);
// Trim whitespace on both ends of a string.
extern std::string Trim(const std::string& s);

// Search for occurrances of a string search_string in s, case insensitive.
// Returns the first occurrance after the given position, or string::npos if not
// found.
extern std::string::size_type CaseInsensitiveSearch(
    const std::string& s,
    const std::string& search_string,
    std::string::size_type pos = 0);

#endif
