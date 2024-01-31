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

mux_result_t muxCreateBuffer(mux_device_t device, size_t size,
                             mux_allocator_info_t allocator_info,
                             mux_buffer_t *out_memory) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (0 == size) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (nullptr == out_memory) {
    return mux_error_null_out_parameter;
  }

  const mux_result_t error =
      muxSelectCreateBuffer(device, size, allocator_info, out_memory);

  if (mux_success == error) {
    mux::setId<mux_object_id_buffer>(device->info->id, *out_memory);
  }

  return error;
}

void muxDestroyBuffer(mux_device_t device, mux_buffer_t memory,
                      mux_allocator_info_t allocator_info) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return;
  }

  if (mux::objectIsInvalid(memory)) {
    return;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return;
  }

  muxSelectDestroyBuffer(device, memory, allocator_info);
}

mux_result_t muxBindBufferMemory(mux_device_t device, mux_memory_t memory,
                                 mux_buffer_t buffer, uint64_t offset) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(memory)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(buffer)) {
    return mux_error_invalid_value;
  }

  if (memory->size < offset) {
    return mux_error_invalid_value;
  }

  if (memory->size < (buffer->memory_requirements.size + offset)) {
    return mux_error_invalid_value;
  }

  return muxSelectBindBufferMemory(device, memory, buffer, offset);
}
