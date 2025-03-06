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

#ifndef _CLIK_CLIK_COMMON_H
#define _CLIK_CLIK_COMMON_H

#include <stdint.h>

#if defined _WIN32 || defined __CYGWIN__
#define CLIK_DLL_IMPORT __declspec(dllimport)
#define CLIK_DLL_EXPORT __declspec(dllexport)
#else
#define CLIK_DLL_IMPORT __attribute__((visibility("default")))
#define CLIK_DLL_EXPORT __attribute__((visibility("default")))
#endif

#ifdef CLIK_DLL
#ifdef CLIK_DLL_EXPORTS
#define CLIK_API CLIK_DLL_EXPORT
#else
#define CLIK_API CLIK_DLL_IMPORT
#endif
#else
#define CLIK_API
#endif

struct clik_buffer;

struct CLIK_API clik_ndrange {
  static constexpr uint32_t max_dimensions = 3;
  // Origin of the work 'grid'. Used when the first item is not at (0, 0, 0).
  uint64_t offset[max_dimensions];
  // Global size, i.e. total number of work-items in each dimension.
  uint64_t global[max_dimensions];
  // Local size, i.e. size of a work-group in each dimension.
  uint64_t local[max_dimensions];
  // Number of dimensions to use.
  uint32_t dims;
};

// Identifies the type of a kernel argument.
enum class CLIK_API clik_argument_type {
  // Not a valid type of argument.
  invalid = 0,
  // Used to pass a clik_buffer object to a kernel as a global pointer.
  buffer = 1,
  // Used to pass a scalar value to a kernel by value.
  byval = 2,
  // Used to pass a chunk of shared memory to a kernel as a local pointer.
  local = 3
};

// Represents a value that will be passed to a kernel as an argument.
struct CLIK_API clik_argument {
  // Type of argument this value contains.
  clik_argument_type type;
  // For buffer arguments, the actual buffer object to pass to the kernel.
  clik_buffer *buffer;
  // For 'byval' arguments, the size of the value to pass to the kernel.
  // For 'local' arguments, the size of the local buffer to allocate.
  uint64_t size;
  // For 'byval' arguments, contents of the value to pass to the kernel.
  uint8_t *contents;
};

// Initialize a kernel argument with a buffer value.
CLIK_API void clik_init_buffer_arg(clik_argument *arg, clik_buffer *buffer);

// Initialize a kernel argument with a scalar value.
CLIK_API void clik_init_scalar_arg(clik_argument *arg, const void *val,
                                   uint64_t size);

template <typename T>
CLIK_API void clik_init_scalar_arg(clik_argument *arg, T &val) {
  // This relies on val not going out of scope before arg does.
  clik_init_scalar_arg(arg, &val, sizeof(T));
}

// Initialize a kernel argument with a local memory value.
CLIK_API void clik_init_local_memory_arg(clik_argument *arg, uint64_t size);

// Initialize a N-D range value with a 2-dimensional range.
CLIK_API void clik_init_ndrange_2d(clik_ndrange *ndrange, uint64_t global_x,
                                   uint64_t global_y, uint64_t local_x,
                                   uint64_t local_y);

// Initialize a N-D range value with a 1-dimensional range.
CLIK_API void clik_init_ndrange_1d(clik_ndrange *ndrange, uint64_t global_size,
                                   uint64_t local_size);

// Initialize a N-D range value with a 2-dimensional range.
CLIK_API void clik_init_ndrange_2d(clik_ndrange *ndrange, uint64_t global_x,
                                   uint64_t global_y, uint64_t local_x,
                                   uint64_t local_y);

#endif  // _CLIK_CLIK_SYNC_API_H
