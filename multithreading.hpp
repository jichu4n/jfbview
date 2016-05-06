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

// This file declares a simple abstraction for parallel execution.

#ifndef MULTITHREADING_HPP
#define MULTITHREADING_HPP

#include <functional>

// Returns the sane default number of threads.
extern int GetDefaultNumThreads();

// Execute f in parallel. f is invoked with arguments (n, i) where n is the
// total number of threads, and i is the index of the current thread (0-based).
// num_threads specifies how many threads to spawn, and defaults to the number
// of CPU cores. Blocks until all spawned threads exit.
extern void ExecuteInParallel(
    const std::function<void(int, int)> &f,
    int num_threads = 0);

#endif

