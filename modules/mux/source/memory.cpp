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

mux_result_t muxAllocateMemory(mux_device_t device, size_t size, uint32_t heap,
                               uint32_t memory_properties,
                               mux_allocation_type_e allocation_type,
                               uint32_t alignment,
                               mux_allocator_info_t allocator_info,
                               mux_memory_t *out_memory) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (0 == size) {
    return mux_error_invalid_value;
  }

  // NOTE: Determine if an invalid value has been passed to the
  // memory_properties bitfield by creating a mask of all valid
  // mux_memory_property_e's. If an additional value is added to the
  // mux_memory_propert_e enum, bitwise or it with existing values.
  if (memory_properties &
      ~(mux_memory_property_device_local | mux_memory_property_host_visible |
        mux_memory_property_host_coherent | mux_memory_property_host_cached)) {
    return mux_error_invalid_value;
  }

  if (0 == memory_properties) {
    return mux_error_invalid_value;
  }

  switch (allocation_type) {
    default:
      return mux_error_invalid_value;
    case mux_allocation_type_alloc_host:
    case mux_allocation_type_alloc_device:
      break;
  }

  // Checks for 0 or power-of-two
  if ((alignment & (alignment - 1)) != 0) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (nullptr == out_memory) {
    return mux_error_null_out_parameter;
  }

  const mux_result_t error = muxSelectAllocateMemory(
      device, size, heap, memory_properties, allocation_type, alignment,
      allocator_info, out_memory);

  if (mux_success == error) {
    mux::setId<mux_object_id_memory>(device->info->id, *out_memory);
  }

  return error;
}

mux_result_t muxCreateMemoryFromHost(mux_device_t device, size_t size,
                                     void *host_pointer,
                                     mux_allocator_info_t allocator_info,
                                     mux_memory_t *out_memory) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (nullptr == out_memory) {
    return mux_error_null_out_parameter;
  }

  if (nullptr == host_pointer) {
    return mux_error_invalid_value;
  }

  if (0 == size) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (!(device->info->allocation_capabilities &
        mux_allocation_capabilities_cached_host)) {
    return mux_error_feature_unsupported;
  }

  const mux_result_t error = muxSelectCreateMemoryFromHost(
      device, size, host_pointer, allocator_info, out_memory);

  if (mux_success == error) {
    mux::setId<mux_object_id_memory>(device->info->id, *out_memory);
  }

  return error;
}

void muxFreeMemory(mux_device_t device, mux_memory_t memory,
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

  muxSelectFreeMemory(device, memory, allocator_info);
}

mux_result_t muxMapMemory(mux_device_t device, mux_memory_t memory,
                          uint64_t offset, uint64_t size, void **out_data) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(memory)) {
    return mux_error_invalid_value;
  }

  if (memory->size < (offset + size)) {
    return mux_error_invalid_value;
  }

  if (memory->properties & mux_memory_property_device_local) {
    return mux_error_invalid_value;
  }

  if (0 == size) {
    return mux_error_invalid_value;
  }

  if (nullptr == out_data) {
    return mux_error_null_out_parameter;
  }

  return muxSelectMapMemory(device, memory, offset, size, out_data);
}

mux_result_t muxFlushMappedMemoryToDevice(mux_device_t device,
                                          mux_memory_t memory, uint64_t offset,
                                          uint64_t size) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(memory)) {
    return mux_error_invalid_value;
  }

  if (memory->size < (offset + size)) {
    return mux_error_invalid_value;
  }

  return muxSelectFlushMappedMemoryToDevice(device, memory, offset, size);
}

mux_result_t muxFlushMappedMemoryFromDevice(mux_device_t device,
                                            mux_memory_t memory,
                                            uint64_t offset, uint64_t size) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(memory)) {
    return mux_error_invalid_value;
  }

  if (memory->size < (offset + size)) {
    return mux_error_invalid_value;
  }

  return muxSelectFlushMappedMemoryFromDevice(device, memory, offset, size);
}

mux_result_t muxUnmapMemory(mux_device_t device, mux_memory_t memory) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(memory)) {
    return mux_error_invalid_value;
  }

  return muxSelectUnmapMemory(device, memory);
}
