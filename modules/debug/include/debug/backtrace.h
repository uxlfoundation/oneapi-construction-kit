// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/// @file
///
/// @brief Print a backtrace of the call stack.

#ifndef DEBUG_BACKTRACE_H_INCLUDED
#define DEBUG_BACKTRACE_H_INCLUDED

#ifndef DEBUG_BACKTRACE_ENABLED
#error debug/backtrace.h can not be included if CA_ENABLE_DEBUG_BACKTRACE=OFF
#endif

#include <cstdio>

/// @brief Print a backtrace of the call stack with file, line info.
#define DEBUG_BACKTRACE                                                        \
  {                                                                            \
    std::fprintf(stderr, "backtrace from %s:%d\n", __FILE__, __LINE__);        \
    debug::print_backtrace(stderr);                                            \
  }

namespace debug {
/// @brief Print a backtrace of the current frame to a file.
///
/// @param out Output destination for the backtrace.
void print_backtrace(FILE *out);
} // namespace debug

#endif // DEBUG_BACKTRACE_H_INCLUDED
