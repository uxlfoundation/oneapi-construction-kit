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
/// @brief Utility macros used to implement the OpenCL API.

#ifndef CL_MACROS_H_INCLUDED
#define CL_MACROS_H_INCLUDED

#include <cstdio>
#include <cstdlib>

#ifndef NDEBUG
/// @brief Assert that a condition is true or display message and abort.
///
/// Much like the C assert standard library macro OCL_ASSERT checks `CONDITION`
/// to verify that it is true before continuing execution. If `CONDITION` is
/// false then @p MESSAGE is displayed along with the source location before
/// aborting execution.
///
/// @param CONDITION Condition to test.
/// @param MESSAGE Message to display on assertion failure.
#define OCL_ASSERT(CONDITION, MESSAGE)                                         \
  if (!(CONDITION)) {                                                          \
    (void)std::fprintf(stderr, "%s:%d: %s %s\n", __FILE__, __LINE__,           \
                       #CONDITION, MESSAGE);                                   \
    std::abort();                                                              \
  }                                                                            \
  (void)0
#else
#define OCL_ASSERT(CONDITION, MESSAGE) (void)0
#endif

/// @brief Display a message to stderr and abort.
///
/// @param MESSAGE Message to display prior to aborting.
#define OCL_ABORT(MESSAGE)                                                     \
  {                                                                            \
    (void)std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, MESSAGE);    \
    std::abort();                                                              \
  }                                                                            \
  (void)0

/// @brief Provides hints to the compiler about the likelihood of a condition
/// returning true.
///
/// Can probably be replaced with [[likely]] and [[unlikely]] in C++20.
#if defined(__GNUC__) || defined(__clang__)
#define OCL_LIKELY(EXPR) __builtin_expect((bool)(EXPR), true)
#define OCL_UNLIKELY(EXPR) __builtin_expect((bool)(EXPR), false)
#else
#define OCL_LIKELY(EXPR) (EXPR)
#define OCL_UNLIKELY(EXPR) (EXPR)
#endif

/// @brief      OCL_CHECK is mainly used to perform OpenCL API parameter
///             validation checking.
/// @param      CONDITION - A bool condition.
/// @param      ACTION - The code to execute on bool condition being true.
#define OCL_CHECK(CONDITION, ACTION)                                           \
  if (OCL_UNLIKELY(CONDITION)) {                                               \
    ACTION;                                                                    \
  }                                                                            \
  (void)0

/// @brief      OCL_SET_IF_NOT_NULL is mainly used to check OpenCL API
///             parameters when pointer types to know to return a value
///             via that object if given.
/// @param      POINTER - A bool condition compare pointer to NULL.
/// @param      VALUE - Assign the object pointed to by the pointer a value.
#define OCL_SET_IF_NOT_NULL(POINTER, VALUE)                                    \
  if ((POINTER) != nullptr) {                                                  \
    *(POINTER) = VALUE;                                                        \
  }                                                                            \
  (void)0

#define OCL_UNUSED(x) ((void)(x))

/// @brief Extension version when in OpenCL-3.0 mode.
///
/// Useful macro for reducing boilerplate when conditionally defining
/// extension versions.
#if defined(CL_VERSION_3_0)
#define CA_CL_EXT_VERSION(MAJOR, MINOR, PATCH)                                 \
  , CL_MAKE_VERSION_KHR(MAJOR, MINOR, PATCH)
#else
#define CA_CL_EXT_VERSION(MAJOR, MINOR, PATCH)
#endif

#endif // CL_MACROS_H_INCLUDED
