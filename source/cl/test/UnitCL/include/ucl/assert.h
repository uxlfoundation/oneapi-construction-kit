// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
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
/// @brief

#ifndef UCL_ASSERT_H_INCLUDED
#define UCL_ASSERT_H_INCLUDED

#include <cstdio>
#include <cstdlib>

#define UCL_ASSERT(CONDITION, MESSAGE)                                       \
  if (!(CONDITION)) {                                                        \
    (void)std::fprintf(stderr, "%s: %d: %s\n", __FILE__, __LINE__, MESSAGE); \
    std::abort();                                                            \
  }

#define UCL_ABORT(FORMAT, ...)                                          \
  {                                                                     \
    (void)std::fprintf(stderr, "abort: %s: %d: " FORMAT "\n", __FILE__, \
                       __LINE__, __VA_ARGS__);                          \
    std::abort();                                                       \
  }

#endif  // UCL_ASSERT_H_INCLUDED
