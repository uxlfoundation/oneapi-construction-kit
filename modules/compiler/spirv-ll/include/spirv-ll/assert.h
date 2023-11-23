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

#ifndef SPIRV_LL_SPV_ASSERT_INCLUDED_H
#define SPIRV_LL_SPV_ASSERT_INCLUDED_H

#if defined(_MSC_VER) && _MSC_VER < 1900
#define SPIRV_LL_FUNCTION __FUNCTION__
#else
#define SPIRV_LL_FUNCTION __func__
#endif

#ifndef NDEBUG
#include <cstdio>
#include <cstdlib>

#define SPIRV_LL_ABORT(MESSAGE)                                                \
  (void)std::fprintf(stderr, "%s:%d: %s\n  %s\n", __FILE__, __LINE__, MESSAGE, \
                     SPIRV_LL_FUNCTION);                                       \
  std::abort()

#define SPIRV_LL_ASSERT(CONDITION, MESSAGE) \
  if (!(CONDITION)) {                       \
    SPIRV_LL_ABORT(MESSAGE);                \
  }                                         \
  (void)0

#define SPIRV_LL_ASSERT_PTR(POINTER)     \
  if (nullptr == (POINTER)) {            \
    SPIRV_LL_ABORT(#POINTER " is null"); \
  }                                      \
  (void)0
#else
#define SPIRV_LL_ABORT(MESSAGE)
#define SPIRV_LL_ASSERT(CONDITION, MESSAGE)
#define SPIRV_LL_ASSERT_PTR(POINTER)
#endif

#endif  // SPIRV_LL_SPV_ASSERT_INCLUDED_H
