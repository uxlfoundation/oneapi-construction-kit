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

#include <CL/cl.h>
#include <cargo/expected.h>
#include <cl/device.h>
#include <cl/semaphore.h>
#include <mux/mux.h>

cargo::expected<mux_shared_semaphore, cl_int> _mux_shared_semaphore::create(
    cl_device_id device, mux_semaphore_t semaphore) {
  std::unique_ptr<_mux_shared_semaphore> sem(
      new _mux_shared_semaphore(device, semaphore));
  if (!sem) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  return sem.release();
}

_mux_shared_semaphore::~_mux_shared_semaphore() {
  muxDestroySemaphore(device->mux_device, semaphore, device->mux_allocator);
}

cl_int _mux_shared_semaphore::retain() {
  const cl_uint last_ref_count = ref_count;
  cl_uint next_ref_count;
  OCL_ASSERT(0u != last_ref_count,
             "Cannot retain object with internal reference count of zero.");
  next_ref_count = last_ref_count + 1;
  // Check for overflow.
  if (next_ref_count < last_ref_count) {
    return CL_OUT_OF_RESOURCES;
  }
  ref_count = next_ref_count;
  return CL_SUCCESS;
}

bool _mux_shared_semaphore::release() {
  const cl_uint last_ref_count = ref_count;

  OCL_ASSERT(0u < last_ref_count,
             "Cannot release object with internal reference count of zero.");
  ref_count = last_ref_count - 1;

  if (0u == ref_count) {
    return true;
  }
  return false;
}
