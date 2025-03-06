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
/// @brief Semaphore support for use in OpenCL command queue API.

#include <CL/cl.h>
#include <cargo/expected.h>
#include <mux/mux.h>

#ifndef CL_SEMAPHORE_H_INCLUDED
#define CL_SEMAPHORE_H_INCLUDED

typedef struct _mux_shared_semaphore *mux_shared_semaphore;

/// @brief A shared wrapper for a semaphore, allowing references across queues
/// @note This is not thread safe and should only be used when there is a
/// command queue mutex.
struct _mux_shared_semaphore final {
 private:
  cl_device_id device;

  _mux_shared_semaphore(cl_device_id device, mux_semaphore_t semaphore)
      : device(device), ref_count(1), semaphore(semaphore) {};
  cl_uint ref_count;

 public:
  mux_semaphore_t semaphore;
  static cargo::expected<mux_shared_semaphore, cl_int> create(
      cl_device_id device, mux_semaphore_t semaphore);
  ~_mux_shared_semaphore();

  /// @brief Increment the semaphore's reference count
  /// @note This is not thread safe and should only be done inside a command
  /// buffer mutex.
  /// @return CL_SUCCESS on success, CL_OUT_OF_RESOURCES if retain results in an
  /// overflow.
  cl_int retain();

  /// @brief Decrement the semaphore internal reference count.
  ///
  ///
  /// @return Returns true if the object can be destroyed,
  /// false otherwise.
  bool release();

  /// @brief return underlying mux semaphore
  mux_semaphore_t get() const { return semaphore; }
};

#endif
