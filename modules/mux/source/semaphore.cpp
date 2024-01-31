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

#include "mux/select.h"
#include "mux/utils/id.h"
#include "tracer/tracer.h"

mux_result_t muxCreateSemaphore(mux_device_t device,
                                mux_allocator_info_t allocator_info,
                                mux_semaphore_t *out_semaphore) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (nullptr == out_semaphore) {
    return mux_error_null_out_parameter;
  }

  const mux_result_t error =
      muxSelectCreateSemaphore(device, allocator_info, out_semaphore);

  if (mux_success == error) {
    mux::setId<mux_object_id_semaphore>(device->info->id, *out_semaphore);
  }

  return error;
}

void muxDestroySemaphore(mux_device_t device, mux_semaphore_t semaphore,
                         mux_allocator_info_t allocator_info) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return;
  }

  if (mux::objectIsInvalid(semaphore)) {
    return;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return;
  }

  muxSelectDestroySemaphore(device, semaphore, allocator_info);
}

mux_result_t muxResetSemaphore(mux_semaphore_t semaphore) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(semaphore)) {
    return mux_error_invalid_value;
  }

  return muxSelectResetSemaphore(semaphore);
}
