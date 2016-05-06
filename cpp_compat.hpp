/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012-2016 Chuan Ji <ji@chu4n.com>                          *
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

// This file defines C++ compatibility helpers to support older compiler and
// standard library versions.

#ifndef CPP_COMPAT_HPP
#define CPP_COMPAT_HPP

// Implement std::make_unique for GCC < 4.9.
#ifdef __GNUC__
#  if (__GNUC__ == 4 && __GNUC_MINOR < 9)
namespace std {

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
      return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}
#  endif
#endif

#endif

