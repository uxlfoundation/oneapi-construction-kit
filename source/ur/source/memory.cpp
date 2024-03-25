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

#include "ur/memory.h"

#include <cassert>
#include <cstdlib>

#include "mux/mux.hpp"
#include "mux/utils/helpers.h"
#include "ur/context.h"
#include "ur/mux.h"
#include "ur/platform.h"

ur_mem_handle_t_::~ur_mem_handle_t_() {
  switch (type) {
    case UR_MEM_TYPE_BUFFER:
      for (size_t index = 0; index < context->devices.size(); index++) {
        muxDestroyBuffer(context->devices[index]->mux_device,
                         buffers[index].mux_buffer,
                         context->platform->mux_allocator_info);
        muxFreeMemory(context->devices[index]->mux_device,
                      buffers[index].mux_memory,
                      context->platform->mux_allocator_info);
      }
      buffers.~small_vector();
      break;
    default:
      (void)std::fprintf(stderr,
                         "ur_mem_handle_t unimplemented mem type destructor");
      std::abort();
  }
}

cargo::expected<ur_mem_handle_t, ur_result_t> ur_mem_handle_t_::createBuffer(
    ur_context_handle_t hContext, ur_mem_flags_t flags, size_t size,
    void *hostPtr) {
  cargo::small_vector<ur::device_buffer_t, 4> buffers;
  if (buffers.reserve(hContext->devices.size())) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }

  switch (flags) {
    // TODO: OpenCL allows 0 but there's no mention of this "default" in the
    // Unified Runtime spec.
    case UR_MEM_FLAG_READ_WRITE:
    case UR_MEM_FLAG_WRITE_ONLY:
    case UR_MEM_FLAG_READ_ONLY: {
      for (auto device : hContext->devices) {
        mux::unique_ptr<mux_buffer_t> mux_buffer{
            nullptr, {nullptr, {nullptr, nullptr, nullptr}}};
        {
          mux_buffer_t buffer;
          if (auto error = muxCreateBuffer(device->mux_device, size,
                                           device->platform->mux_allocator_info,
                                           &buffer)) {
            return cargo::make_unexpected(ur::resultFromMux(error));
          }
          mux_buffer = mux::unique_ptr<mux_buffer_t>(
              buffer,
              {device->mux_device, device->platform->mux_allocator_info});
        }
        auto heap = mux::findFirstSupportedHeap(
            mux_buffer->memory_requirements.supported_heaps);
        mux::unique_ptr<mux_memory_t> mux_memory{
            nullptr, {nullptr, {nullptr, nullptr, nullptr}}};
        {
          // TODO: The properties and allocation type will need another look
          // after the MVP has been realised.
          mux_memory_t memory;
          if (auto error = muxAllocateMemory(
                  device->mux_device, size, heap,
                  mux_memory_property_host_visible,
                  mux_allocation_type_alloc_device,
                  mux_buffer->memory_requirements.alignment,
                  device->platform->mux_allocator_info, &memory)) {
            return cargo::make_unexpected(ur::resultFromMux(error));
          }
          mux_memory = mux::unique_ptr<mux_memory_t>(
              memory,
              {device->mux_device, device->platform->mux_allocator_info});
        }
        if (auto error = muxBindBufferMemory(
                device->mux_device, mux_memory.get(), mux_buffer.get(), 0)) {
          return cargo::make_unexpected(ur::resultFromMux(error));
        }
        if (buffers.push_back(ur::device_buffer_t{mux_buffer.release(),
                                                  mux_memory.release()})) {
          return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
        }
      }
    } break;

    case UR_MEM_FLAG_USE_HOST_POINTER:
    case UR_MEM_FLAG_ALLOC_HOST_POINTER:
    case UR_MEM_FLAG_ALLOC_COPY_HOST_POINTER:
    default:
      (void)hostPtr;
      (void)std::fprintf(stderr,
                         "urMemBufferCreate ur_mem_flags_t not implemented\n");
      std::abort();
  }

  auto buffer = std::make_unique<ur_mem_handle_t_>(
      hContext, UR_MEM_TYPE_BUFFER, flags, std::move(buffers), size);
  if (!buffer) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }
  return buffer.release();
}

ur_result_t ur_mem_handle_t_::sync(ur_queue_handle_t command_queue) {
  // We only need to enforce memory consistency if there is more than 1 device
  // in the context.
  if (context->devices.size() <= 1) {
    return UR_RESULT_SUCCESS;
  }

  // If the memory has unititlized last_command_queue it means this is the first
  // memory command to operate on that buffer, in which case all we need to do
  // is cache the command queue that modifies the memory and we are done.
  if (!last_command_queue) {
    last_command_queue = command_queue;
    return UR_RESULT_SUCCESS;
  }

  // We only actually need to synchronize memory when the last command to access
  // the memory was on a different queue than the current command. e.g. for two
  // command queue_s q_a and q_b if q_a write to memory then q_b reads we need
  // to sync the memory before the read, however if q_a writes then q_a reads,
  // there is no requirement to sync the memory since it will already be
  // consistent.
  if (last_command_queue == command_queue) {
    return UR_RESULT_SUCCESS;
  }

  // If we got to this point then we actually need to synchronize the underlying
  // memory of the mux devices.
  // Perform the synchronization.
  const auto mux_src_device = last_command_queue->device->mux_device;
  const auto src_device_idx = last_command_queue->getDeviceIdx();
  const auto mux_src_memory = buffers[src_device_idx].mux_memory;

  const auto mux_dst_device = command_queue->device->mux_device;
  const auto dst_device_idx = command_queue->getDeviceIdx();
  const auto mux_dst_memory = buffers[dst_device_idx].mux_memory;

  if (auto mux_error = mux::synchronizeMemory(
          mux_src_device, mux_dst_device, mux_src_memory, mux_dst_memory,
          /* host_ptr_source */ nullptr, /* host_ptr_source */ nullptr,
          /* offset */ 0, size)) {
    return ur::resultFromMux(mux_error);
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL
urMemBufferCreate(ur_context_handle_t hContext, ur_mem_flags_t flags,
                  size_t size, void *hostPtr, ur_mem_handle_t *phBuffer) {
  if (!hContext) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (!ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }
  if (hContext->platform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_INVALID_CONTEXT;
  }
  if (0x3f < flags) {
    return UR_RESULT_ERROR_INVALID_ENUMERATION;
  }
  if (size == 0 && "size must not be zero") {
    return UR_RESULT_ERROR_INVALID_VALUE;
  }
  // TODO: The spec states hostPtr is not optional, yet it would only be used
  // when certain flags are present, thus it should be allowed to be nullptr.
  if (!phBuffer) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }
  auto buffer = ur_mem_handle_t_::createBuffer(hContext, flags, size, hostPtr);
  if (!buffer) {
    return buffer.error();
  }
  *phBuffer = *buffer;
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urMemRetain(ur_mem_handle_t hMem) {
  if (!hMem) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::retain(hMem);
}

UR_APIEXPORT ur_result_t UR_APICALL urMemRelease(ur_mem_handle_t hMem) {
  if (!hMem) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::release(hMem);
}

UR_APIEXPORT ur_result_t UR_APICALL urUSMHostAlloc(ur_context_handle_t hContext,
                                                   ur_usm_desc_t *pUSMDesc,
                                                   ur_usm_pool_handle_t pool,
                                                   size_t size, uint32_t align,
                                                   void **pptr) {
  // TODO: handle pool
  (void)pool;
  if (!hContext) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }

  if (hContext->platform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_INVALID_CONTEXT;
  }

  if (!pptr) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (size == 0) {
    return UR_RESULT_ERROR_INVALID_USM_SIZE;
  }

  ur_usm_mem_flags_t flags = 0;
  if (pUSMDesc) {
    flags = pUSMDesc->flags;
  }

  const std::lock_guard<std::mutex> lock_guard(hContext->mutex);
  auto host_allocation =
      std::make_unique<ur::host_allocation_info>(hContext, flags, size, align);
  if (host_allocation->allocate()) {
    return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  }
  if (hContext->usm_allocations.push_back(std::move(host_allocation))) {
    return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  }
  *pptr = hContext->usm_allocations.back()->base_ptr;

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urUSMFree(ur_context_handle_t hContext,
                                              void *ptr) {
  if (!hContext) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!ptr) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  auto is_usm_ptr = [&](const std::unique_ptr<ur::allocation_info> &usm_alloc) {
    return usm_alloc->base_ptr == ptr;
  };

  const std::lock_guard<std::mutex> lock_guard(hContext->mutex);
  auto usm_alloc_iterator =
      std::find_if(hContext->usm_allocations.begin(),
                   hContext->usm_allocations.end(), is_usm_ptr);
  if (usm_alloc_iterator == hContext->usm_allocations.end()) {
    return UR_RESULT_ERROR_INVALID_MEM_OBJECT;
  }

  hContext->usm_allocations.erase(usm_alloc_iterator);

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL
urUSMDeviceAlloc(ur_context_handle_t hContext, ur_device_handle_t device,
                 ur_usm_desc_t *pUSMDesc, ur_usm_pool_handle_t pool,
                 size_t size, uint32_t align, void **pptr) {
  // TODO: handle pool
  (void)pool;
  if (!hContext) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!device) {
    return UR_RESULT_ERROR_INVALID_DEVICE;
  }

  if (std::find(hContext->devices.begin(), hContext->devices.end(), device) ==
      hContext->devices.end()) {
    return UR_RESULT_ERROR_INVALID_DEVICE;
  }

  if (!ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }

  if (hContext->platform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_INVALID_CONTEXT;
  }

  if (!pptr) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (size == 0) {
    return UR_RESULT_ERROR_INVALID_USM_SIZE;
  }

  ur_usm_mem_flags_t flags = 0;
  if (pUSMDesc) {
    flags = pUSMDesc->flags;
  }

  const std::lock_guard<std::mutex> lock_guard(hContext->mutex);
  auto device_allocation = std::make_unique<ur::device_allocation_info>(
      hContext, device, flags, size, align);
  if (device_allocation->allocate()) {
    return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  }

  if (hContext->usm_allocations.push_back(std::move(device_allocation))) {
    return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  }
  *pptr = hContext->usm_allocations.back()->base_ptr;

  return UR_RESULT_SUCCESS;
}
