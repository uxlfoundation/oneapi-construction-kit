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

#include "ur/context.h"

#include <algorithm>
#include <cassert>

#include "ur/device.h"
#include "ur/platform.h"

ur::host_allocation_info::host_allocation_info(ur_context_handle_t context,
                                               const ur_usm_mem_flags_t USMFlag,
                                               size_t size, uint32_t alignment)
    : ur::allocation_info(context, USMFlag, size, alignment) {
  uint32_t max_align = 0;
  for (auto device : context->devices) {
    const auto device_align = device->mux_device->info->buffer_alignment;
    max_align = std::max(device_align, max_align);
  }

  if (alignment == 0) {
    align = max_align;
  }
}

ur_result_t ur::host_allocation_info::allocate() {
  base_ptr = cargo::alloc(size, align);
  if (!base_ptr) {
    return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  }

  const size_t num_devices = context->devices.size();
  if (cargo::success != mux_memories.alloc(num_devices)) {
    return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  }

  if (cargo::success != mux_buffers.alloc(num_devices)) {
    return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  }

  for (size_t i = 0; i < num_devices; i++) {
    auto device = context->devices[i];
    if (!device->supportsHostAllocations()) {
      // Device doesn't support host allocations
      continue;
    }

    if (auto error = muxCreateBuffer(device->mux_device, size,
                                     device->platform->mux_allocator_info,
                                     &mux_buffers[i])) {
      return ur::resultFromMux(error);
    }

    if (auto error = muxCreateMemoryFromHost(
            device->mux_device, size, base_ptr,
            device->platform->mux_allocator_info, &mux_memories[i])) {
      return ur::resultFromMux(error);
    }

    if (auto error = muxBindBufferMemory(device->mux_device, mux_memories[i],
                                         mux_buffers[i], 0 /*Offset*/)) {
      return ur::resultFromMux(error);
    }
  }
  return UR_RESULT_SUCCESS;
}

mux_buffer_t ur::host_allocation_info::getMuxBufferForDevice(
    ur_device_handle_t query_device) const {
  auto device_index = context->getDeviceIdx(query_device);
  if (auto mux_buffer = mux_buffers.at(device_index)) {
    return *mux_buffer;
  }
  return nullptr;
}

ur::host_allocation_info::~host_allocation_info() {
  for (uint32_t index = 0; index < context->devices.size(); ++index) {
    auto device = context->devices[index];

    mux_buffer_t mux_buffer = mux_buffers[index];
    if (mux_buffer) {
      muxDestroyBuffer(device->mux_device, mux_buffer,
                       device->platform->mux_allocator_info);
    }

    mux_memory_t mux_memory = mux_memories[index];
    if (mux_memory) {
      muxFreeMemory(device->mux_device, mux_memory,
                    device->platform->mux_allocator_info);
    }
  }

  // Free the host side allocation
  if (base_ptr) {
    cargo::free(base_ptr);
  }
}

ur::device_allocation_info::device_allocation_info(ur_context_handle_t context,
                                                   ur_device_handle_t device,
                                                   ur_usm_mem_flags_t USMProp,
                                                   size_t size, uint32_t align)
    : ur::allocation_info(context, USMProp, size, align), device(device) {}

ur_result_t ur::device_allocation_info::allocate() {
  // It should be power of 2
  if ((align & (align - 1)) != 0) {
    return UR_RESULT_ERROR_INVALID_VALUE;
  }

  const auto device_align = device->mux_device->info->buffer_alignment;
  if (align == 0) {
    align = device_align;
  } else if (align > device_align) {
    return UR_RESULT_ERROR_INVALID_VALUE;
  }

  const auto max_alloc_size = device->mux_device->info->allocation_size;
  if (size > max_alloc_size) {
    return UR_RESULT_ERROR_INVALID_USM_SIZE;
  }

  constexpr uint32_t heap = 1;
  if (auto error = muxAllocateMemory(
          device->mux_device, size, heap, mux_memory_property_device_local,
          mux_allocation_type_alloc_device, align,
          device->platform->mux_allocator_info, &mux_memory)) {
    return ur::resultFromMux(error);
  }

  if (auto error =
          muxCreateBuffer(device->mux_device, size,
                          device->platform->mux_allocator_info, &mux_buffer)) {
    return ur::resultFromMux(error);
  }

  if (auto error =
          muxBindBufferMemory(device->mux_device, mux_memory, mux_buffer, 0)) {
    return ur::resultFromMux(error);
  }
#if INTPTR_MAX == INT64_MAX
  base_ptr = reinterpret_cast<void *>(mux_memory->handle);
#elif INTPTR_MAX == INT32_MAX
  assert((device->mux_device->info->address_capabilities &
          mux_address_capabilities_bits32) != 0 &&
         "32-bit host with 64-bit device not supported");
  base_ptr =
      reinterpret_cast<void *>(static_cast<uint32_t>(mux_memory->handle));
#else
#error Unsupported pointer size
#endif

  if (base_ptr == nullptr) {
    return UR_RESULT_ERROR_OUT_OF_RESOURCES;
  }

  return UR_RESULT_SUCCESS;
}

ur::device_allocation_info::~device_allocation_info() {
  if (mux_buffer) {
    muxDestroyBuffer(device->mux_device, mux_buffer,
                     device->platform->mux_allocator_info);
  }

  if (mux_memory) {
    muxFreeMemory(device->mux_device, mux_memory,
                  device->platform->mux_allocator_info);
  }
}

cargo::expected<ur_context_handle_t, ur_result_t> ur_context_handle_t_::create(
    ur_platform_handle_t platform,
    cargo::array_view<const ur_device_handle_t> devices) {
  auto context = std::make_unique<ur_context_handle_t_>(platform);
  if (!context) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }
  if (context->devices.assign(devices.begin(), devices.end())) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }
  return context.release();
}

ur::allocation_info *ur_context_handle_t_::findUSMAllocation(
    const void *base_ptr) {
  if (!base_ptr) {
    return nullptr;
  }

  const std::lock_guard<std::mutex> lock(mutex);
  auto result =
      std::find_if(usm_allocations.begin(), usm_allocations.end(),
                   [base_ptr](std::unique_ptr<ur::allocation_info> &ptr) {
                     return ptr->base_ptr == base_ptr;
                   });
  if (result == usm_allocations.end()) {
    return nullptr;
  }
  return result->get();
}

UR_APIEXPORT ur_result_t UR_APICALL
urContextCreate(uint32_t DeviceCount, const ur_device_handle_t *phDevices,
                const ur_context_properties_t *pProperties,
                ur_context_handle_t *phContext) {
  (void)pProperties;
  if (!phDevices || !phContext) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }
  if (!ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }
  // TODO: There is no relevant error code so this is an assert for now.
  assert(DeviceCount > 0 && "DeviceCount must not be zero");
  assert(std::all_of(phDevices, phDevices + DeviceCount,
                     [&](ur_device_handle_t device) {
                       return device->platform ==
                              ur_platform_handle_t_::instance;
                     }) &&
         "phDevices do not belong to the platform");
  auto context = ur_context_handle_t_::create(
      ur_platform_handle_t_::instance, {phDevices, phDevices + DeviceCount});
  if (!context) {
    return context.error();
  }
  *phContext = *context;
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL
urContextRetain(ur_context_handle_t hContext) {
  if (!hContext) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::retain(hContext);
}

UR_APIEXPORT ur_result_t UR_APICALL
urContextRelease(ur_context_handle_t hContext) {
  if (!hContext) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::release(hContext);
}
