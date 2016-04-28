/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2016 Chuan Ji <ji@chu4n.com>                               *
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

// Print the MuPDF library version as an integer.
//
// Starting from v1.3, MuPDF defines the current version string, such as "1.9a",
// in fitz/version.h. However, that version string is rather difficult to use in
// preprocessor macros for equality and greater-than comparisons. This program
// prints the MuPDF version string as
//
//     majorVersion * 10000 + minorVersion
//
// For example, 1.9a becomes 10009. This value is used to define the
// MUPDF_VERSION macro.

#include <cassert>
#include <iostream>
#include <sstream>
#include "mupdf/fitz.h"

// MuPDF 1.3.66 and above defines the FZ_VERSION macro. However, Ubuntu 14.04
// ships with libmupdf-dev 1.3 which does not define this macro. This will be
// the min version we support, so we will assume 1.3 in the absence of the
// FZ_VERSION macro.
#ifdef FZ_VERSION
  const char* MUPDF_VERSION_STRING = FZ_VERSION;
#else
  const char* MUPDF_VERSION_STRING = "1.3";
#endif


int main() {
  std::istringstream mupdfVersionString(MUPDF_VERSION_STRING);
  int majorVersion, minorVersion;
  char dot;
  mupdfVersionString >> majorVersion >> dot >> minorVersion;
  assert(dot == '.');

  const int intVersion = majorVersion * 10000 + minorVersion;
  std::cout << intVersion << std::endl;

  return 0;
}

