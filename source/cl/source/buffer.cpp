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

#include <cargo/dynamic_array.h>
#include <cl/buffer.h>
#include <cl/command_queue.h>
#include <cl/device.h>
#include <cl/macros.h>
#include <cl/mux.h>
#include <cl/platform.h>
#include <cl/validate.h>
#include <tracer/tracer.h>

#include <algorithm>
#include <cstring>
#include <memory>

#include "mux/utils/helpers.h"

_cl_mem_buffer::_cl_mem_buffer(
    cl_context context, const cl_mem_flags flags, size_t size, void *host_ptr,
    cargo::dynamic_array<mux_memory_t> &&mux_memories,
    cargo::dynamic_array<mux_buffer_t> &&mux_buffers)
    : _cl_mem(context, flags, size, CL_MEM_OBJECT_BUFFER, nullptr, host_ptr,
              cl::ref_count_type::EXTERNAL, std::move(mux_memories)),
      offset(0),
      mux_buffers(std::move(mux_buffers)) {}

_cl_mem_buffer::_cl_mem_buffer(
    const cl_mem_flags flags, const size_t offset, const size_t size,
    cl_mem parent, cargo::dynamic_array<mux_memory_t> &&mux_memories,
    cargo::dynamic_array<mux_buffer_t> &&mux_buffers)
    : _cl_mem(parent->context, flags, size, CL_MEM_OBJECT_BUFFER, parent,
              nullptr, cl::ref_count_type::EXTERNAL, std::move(mux_memories)),
      offset(offset),
      mux_buffers(std::move(mux_buffers)) {
  if (parent->host_ptr) {
    host_ptr = static_cast<char *>(parent->host_ptr) + offset;
  }
}

_cl_mem_buffer::~_cl_mem_buffer() {
  for (size_t index = 0; index < mux_buffers.size(); ++index) {
    muxDestroyBuffer(context->devices[index]->mux_device, mux_buffers[index],
                     context->devices[index]->mux_allocator);
  }
}

cargo::expected<std::unique_ptr<_cl_mem_buffer>, cl_int> _cl_mem_buffer::create(
    cl_context context, cl_mem_flags flags, size_t size, void *host_ptr) {
  cargo::dynamic_array<mux_memory_t> mux_memories;
  cargo::dynamic_array<mux_buffer_t> mux_buffers;
  if (cargo::success != mux_memories.alloc(context->devices.size()) ||
      cargo::success != mux_buffers.alloc(context->devices.size())) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }

  for (cl_uint index = 0; index < context->devices.size(); ++index) {
    OCL_CHECK(size > context->devices[index]->max_mem_alloc_size,
              return cargo::make_unexpected(CL_INVALID_BUFFER_SIZE));
    mux_buffer_t mux_buffer;
    if (muxCreateBuffer(context->devices[index]->mux_device, size,
                        context->devices[index]->mux_allocator, &mux_buffer)) {
      return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
    }
    mux_buffers[index] = mux_buffer;
  }

  std::unique_ptr<_cl_mem_buffer> buffer(new (std::nothrow) _cl_mem_buffer(
      context, flags, size, host_ptr, std::move(mux_memories),
      std::move(mux_buffers)));
  if (!buffer) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }

  for (cl_uint index = 0; index < context->devices.size(); ++index) {
    auto device = context->devices[index];
    mux_device_t mux_device = device->mux_device;
    mux_buffer_t mux_buffer = buffer->mux_buffers[index];

    mux_memory_t mux_memory;
    const cl_int error = buffer->allocateMemory(
        mux_device, mux_buffer->memory_requirements.supported_heaps,
        device->mux_allocator, &mux_memory);
    OCL_CHECK(error, return cargo::make_unexpected(error));
    buffer->mux_memories[index] = mux_memory;

    const uint64_t offset = 0;
    auto mux_error =
        muxBindBufferMemory(mux_device, mux_memory, mux_buffer, offset);
    OCL_CHECK(mux_error,
              return cargo::make_unexpected(CL_MEM_OBJECT_ALLOCATION_FAILURE));

    if (CL_MEM_COPY_HOST_PTR & flags) {
      void *writeTo = nullptr;
      mux_error = muxMapMemory(mux_device, mux_memory, 0, size, &writeTo);
      OCL_CHECK(mux_error, return cargo::make_unexpected(
                               CL_MEM_OBJECT_ALLOCATION_FAILURE));

      std::memcpy(writeTo, host_ptr, size);

      mux_error = muxFlushMappedMemoryToDevice(mux_device, mux_memory, 0, size);
      if ((mux_success != error) ||
          (mux_success != muxUnmapMemory(mux_device, mux_memory))) {
        OCL_CHECK(mux_error, return cargo::make_unexpected(
                                 CL_MEM_OBJECT_ALLOCATION_FAILURE));
      }
    }
  }

  return buffer;
}

cl_int _cl_mem_buffer::synchronize(cl_command_queue command_queue) {
  // Synchronization only required if multiple devices are present in a context.
  if (context->devices.size() > 1) {
    // If this is a sub-buffer use the parent buffer for synchronization.
    const cl_mem_buffer owning_buffer =
        (nullptr == optional_parent)
            ? this
            : static_cast<cl_mem_buffer>(optional_parent);

    // Only synchronize when the last device to update the buffer doesn't match
    // the command queue's device.
    if (owning_buffer->device_owner &&
        owning_buffer->device_owner != command_queue->device) {
      const auto source_device_index =
          context->getDeviceIndex(owning_buffer->device_owner);
      const auto source_mux_device = owning_buffer->device_owner->mux_device;
      const auto source_mux_memory = mux_memories[source_device_index];

      const auto dest_device_index =
          command_queue->context->getDeviceIndex(command_queue->device);
      const auto dest_mux_device = command_queue->device->mux_device;
      const auto dest_mux_memory = mux_memories[dest_device_index];

      // Take a lock on this buffers mutex.
      const std::lock_guard<std::mutex> lock_guard(mutex);

      // Perform the synchronization.
      if (auto mux_error = mux::synchronizeMemory(
              source_mux_device, dest_mux_device, source_mux_memory,
              dest_mux_memory, nullptr, nullptr, /* offset */ 0,
              owning_buffer->size)) {
        return cl::getErrorFrom(mux_error);
      }
    }

    // Update the device owning the synchronized data.
    owning_buffer->device_owner = command_queue->device;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_mem CL_API_CALL cl::CreateBuffer(cl_context context,
                                                 cl_mem_flags flags,
                                                 size_t size, void *host_ptr,
                                                 cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateBuffer");
  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);

  const cl_int error = cl::validate::MemFlags(flags, host_ptr);
  OCL_CHECK(error, OCL_SET_IF_NOT_NULL(errcode_ret, error); return nullptr);

  OCL_CHECK(0 == size, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_BUFFER_SIZE);
            return nullptr);

  auto new_buffer = _cl_mem_buffer::create(context, flags, size, host_ptr);

  if (!new_buffer) {
    OCL_CHECK(new_buffer.error(),
              OCL_SET_IF_NOT_NULL(errcode_ret, new_buffer.error());
              return nullptr);
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return new_buffer->release();
}

CL_API_ENTRY cl_mem CL_API_CALL cl::CreateSubBuffer(
    cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type,
    const void *buffer_create_info, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateSubBuffer");
  OCL_CHECK(!buffer, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_MEM_OBJECT);
            return nullptr);
  const cl_mem_flags rwMask =
      (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY);

  if (!(rwMask & flags)) {
    flags |= (rwMask & buffer->flags);
  }

  const cl_mem_flags ptrMask =
      (CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR);

  OCL_CHECK((ptrMask & flags),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  if (!(ptrMask & flags)) {
    flags |= (ptrMask & buffer->flags);
  }

  const cl_mem_flags hostMask =
      (CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_HOST_NO_ACCESS);

  if (!(hostMask & flags)) {
    flags |= (hostMask & buffer->flags);
  }

  const bool readWrite = cl::validate::IsInBitSet(flags, CL_MEM_READ_WRITE);
  const bool writeOnly = cl::validate::IsInBitSet(flags, CL_MEM_WRITE_ONLY);
  const bool readOnly = cl::validate::IsInBitSet(flags, CL_MEM_READ_ONLY);
  const bool hostWriteOnly =
      cl::validate::IsInBitSet(flags, CL_MEM_HOST_WRITE_ONLY);
  const bool hostReadOnly =
      cl::validate::IsInBitSet(flags, CL_MEM_HOST_READ_ONLY);

  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_WRITE_ONLY) &&
                (readWrite || readOnly),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_READ_ONLY) &&
                (readWrite || writeOnly),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_WRITE_ONLY) &&
                hostReadOnly,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_READ_ONLY) &&
                hostWriteOnly,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_NO_ACCESS) &&
                (hostReadOnly || hostWriteOnly),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(!cl::validate::IsInBitSet(buffer_create_type,
                                      CL_BUFFER_CREATE_TYPE_REGION),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(!buffer_create_info,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  const cl_buffer_region *const info =
      reinterpret_cast<const cl_buffer_region *>(buffer_create_info);
  const size_t origin = info->origin;
  const size_t size = info->size;
  OCL_CHECK((origin + size) > buffer->size,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  OCL_CHECK(0 == info->size,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_BUFFER_SIZE);
            return nullptr);

  OCL_CHECK(std::all_of(buffer->context->devices.begin(),
                        buffer->context->devices.end(),
                        [origin](_cl_device_id *device) {
                          return 0 != (origin &
                                       ((device->mem_base_addr_align / 8) - 1));
                        }),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_MISALIGNED_SUB_BUFFER_OFFSET);
            return nullptr);

  cargo::dynamic_array<mux_memory_t> mux_memories;
  cargo::dynamic_array<mux_buffer_t> mux_buffers;
  if (cargo::success != mux_memories.alloc(buffer->context->devices.size()) ||
      cargo::success != mux_buffers.alloc(buffer->context->devices.size())) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
    return nullptr;
  }

  std::unique_ptr<_cl_mem_buffer> sub_buffer(new (std::nothrow) _cl_mem_buffer(
      flags, origin, size, buffer, std::move(mux_memories),
      std::move(mux_buffers)));
  OCL_CHECK(!sub_buffer,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
            return nullptr);

  for (cl_uint index = 0; index < buffer->context->devices.size(); ++index) {
    auto device = buffer->context->devices[index];

    sub_buffer->mux_memories[index] = buffer->mux_memories[index];

    mux_result_t mux_error =
        muxCreateBuffer(device->mux_device, info->size, device->mux_allocator,
                        &sub_buffer->mux_buffers[index]);
    OCL_CHECK(mux_error, OCL_SET_IF_NOT_NULL(errcode_ret,
                                             CL_MEM_OBJECT_ALLOCATION_FAILURE);
              return nullptr);

    mux_error =
        muxBindBufferMemory(device->mux_device, sub_buffer->mux_memories[index],
                            sub_buffer->mux_buffers[index], origin);
    OCL_CHECK(mux_error, OCL_SET_IF_NOT_NULL(errcode_ret,
                                             CL_MEM_OBJECT_ALLOCATION_FAILURE);
              return nullptr);
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return sub_buffer.release();
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueWriteBufferRect(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
    const size_t *buffer_origin, const size_t *host_origin,
    const size_t *region, size_t buffer_row_pitch, size_t buffer_slice_pitch,
    size_t host_row_pitch, size_t host_slice_pitch, const void *ptr,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueWriteBufferRect");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!buffer, return CL_INVALID_MEM_OBJECT);
  OCL_CHECK(!buffer_origin || !host_origin || !region || !ptr,
            return CL_INVALID_VALUE);

  if (buffer_row_pitch == 0) {
    buffer_row_pitch = region[0];
  }
  if (buffer_slice_pitch == 0) {
    buffer_slice_pitch = region[1] * buffer_row_pitch;
  }
  if (host_row_pitch == 0) {
    host_row_pitch = region[0];
  }
  if (host_slice_pitch == 0) {
    host_slice_pitch = region[1] * host_row_pitch;
  }

  OCL_CHECK(region[0] == 0 || region[1] == 0 || region[2] == 0,
            return CL_INVALID_VALUE);

  OCL_CHECK(region[0] * region[1] * region[2] > buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK(
      buffer_origin[0] * buffer_origin[1] * buffer_origin[2] > buffer->size,
      return CL_INVALID_VALUE);
  OCL_CHECK(((buffer_origin[0] + region[0]) * (buffer_origin[1] + region[1]) *
             (buffer_origin[2] + region[2])) > buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK((((buffer_origin[2] + region[2] - 1) * buffer_slice_pitch) +
             ((buffer_origin[1] + region[1] - 1) * buffer_row_pitch) +
             (buffer_origin[0] + region[0])) > buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK(buffer_row_pitch > buffer->size, return CL_INVALID_VALUE);
  OCL_CHECK(buffer_slice_pitch > buffer->size, return CL_INVALID_VALUE);
  OCL_CHECK(buffer_row_pitch != 0 && buffer_row_pitch < region[0],
            return CL_INVALID_VALUE);
  OCL_CHECK(host_row_pitch != 0 && host_row_pitch < region[0],
            return CL_INVALID_VALUE);
  OCL_CHECK((buffer_slice_pitch != 0) &&
                (buffer_slice_pitch < region[1] * buffer_row_pitch),
            return CL_INVALID_VALUE);
  OCL_CHECK(
      (buffer_slice_pitch != 0) && (buffer_slice_pitch % buffer_row_pitch != 0),
      return CL_INVALID_VALUE);
  OCL_CHECK((host_slice_pitch != 0) &&
                (host_slice_pitch < region[1] * host_row_pitch),
            return CL_INVALID_VALUE);
  OCL_CHECK((host_slice_pitch != 0) && (host_slice_pitch % host_row_pitch != 0),
            return CL_INVALID_VALUE);

  OCL_CHECK(command_queue->context != buffer->context,
            return CL_INVALID_CONTEXT);

  const cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event,
      blocking_write);
  OCL_CHECK(error != CL_SUCCESS, return error);

  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_READ_ONLY),
            return CL_INVALID_OPERATION);
  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_NO_ACCESS),
            return CL_INVALID_OPERATION);

  cl_event return_event = nullptr;
  if (blocking_write || (nullptr != event)) {
    auto new_event =
        _cl_event::create(command_queue, CL_COMMAND_WRITE_BUFFER_RECT);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
  }
  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);

  {
    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer(
        {event_wait_list, num_events_in_wait_list}, event_release_guard.get());
    if (!mux_command_buffer) {
      return CL_OUT_OF_RESOURCES;
    }

    auto device_index = command_queue->getDeviceIndex();
    auto mux_buffer =
        static_cast<cl_mem_buffer>(buffer)->mux_buffers[device_index];

    mux_buffer_region_info_t r_info{
        {region[0], region[1], region[2]},
        {buffer_origin[0], buffer_origin[1], buffer_origin[2]},
        {host_origin[0], host_origin[1], host_origin[2]},
        {buffer_row_pitch, buffer_slice_pitch},
        {host_row_pitch, host_slice_pitch},
    };

    const mux_result_t mux_error = muxCommandWriteBufferRegions(
        *mux_command_buffer, mux_buffer, ptr, &r_info, 1, 0, nullptr, nullptr);

    if (mux_success != mux_error) {
      auto error = cl::getErrorFrom(mux_error);
      if (event_release_guard) {
        event_release_guard->complete(error);
      }
      return error;
    }

    cl::retainInternal(buffer);

    if (auto error =
            static_cast<cl_mem_buffer>(buffer)->synchronize(command_queue)) {
      return error;
    }

    if (auto error = command_queue->registerDispatchCallback(
            *mux_command_buffer, return_event,
            [buffer]() { cl::releaseInternal(buffer); })) {
      return error;
    }
  }

  if (blocking_write) {
    const cl_int ret = cl::WaitForEvents(1, &event_release_guard.get());
    if (CL_SUCCESS != ret) {
      return ret;
    }
  }

  if ((nullptr != event) && event_release_guard) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueReadBufferRect(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
    const size_t *buffer_origin, const size_t *host_origin,
    const size_t *region, size_t buffer_row_pitch, size_t buffer_slice_pitch,
    size_t host_row_pitch, size_t host_slice_pitch, void *ptr,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueReadBufferRect");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!buffer, return CL_INVALID_MEM_OBJECT);
  OCL_CHECK(!buffer_origin || !host_origin || !region || !ptr,
            return CL_INVALID_VALUE);

  if (buffer_row_pitch == 0) {
    buffer_row_pitch = region[0];
  }
  if (buffer_slice_pitch == 0) {
    buffer_slice_pitch = region[1] * buffer_row_pitch;
  }
  if (host_row_pitch == 0) {
    host_row_pitch = region[0];
  }
  if (host_slice_pitch == 0) {
    host_slice_pitch = region[1] * host_row_pitch;
  }

  OCL_CHECK(region[0] == 0 || region[1] == 0 || region[2] == 0,
            return CL_INVALID_VALUE);

  OCL_CHECK(region[0] * region[1] * region[2] > buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK(
      buffer_origin[0] * buffer_origin[1] * buffer_origin[2] > buffer->size,
      return CL_INVALID_VALUE);
  OCL_CHECK(((buffer_origin[0] + region[0]) * (buffer_origin[1] + region[1]) *
             (buffer_origin[2] + region[2])) > buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK((((buffer_origin[2] + region[2] - 1) * buffer_slice_pitch) +
             ((buffer_origin[1] + region[1] - 1) * buffer_row_pitch) +
             (buffer_origin[0] + region[0])) > buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK(buffer_row_pitch > buffer->size, return CL_INVALID_VALUE);
  OCL_CHECK(buffer_slice_pitch > buffer->size, return CL_INVALID_VALUE);
  OCL_CHECK(buffer_row_pitch != 0 && buffer_row_pitch < region[0],
            return CL_INVALID_VALUE);
  OCL_CHECK(host_row_pitch != 0 && host_row_pitch < region[0],
            return CL_INVALID_VALUE);
  OCL_CHECK(buffer_slice_pitch != 0 &&
                buffer_slice_pitch < region[1] * buffer_row_pitch,
            return CL_INVALID_VALUE);
  OCL_CHECK(
      buffer_slice_pitch != 0 && (buffer_slice_pitch % buffer_row_pitch != 0),
      return CL_INVALID_VALUE);
  OCL_CHECK(
      host_slice_pitch != 0 && host_slice_pitch < region[1] * host_row_pitch,
      return CL_INVALID_VALUE);
  OCL_CHECK(host_slice_pitch != 0 && (host_slice_pitch % host_row_pitch != 0),
            return CL_INVALID_VALUE);

  OCL_CHECK(command_queue->context != buffer->context,
            return CL_INVALID_CONTEXT);

  const cl_int error =
      cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                  command_queue->context, event, blocking_read);
  OCL_CHECK(error != CL_SUCCESS, return error);

  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_WRITE_ONLY),
            return CL_INVALID_OPERATION);
  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_NO_ACCESS),
            return CL_INVALID_OPERATION);

  cl_event return_event = nullptr;
  if (blocking_read || (nullptr != event)) {
    auto new_event =
        _cl_event::create(command_queue, CL_COMMAND_READ_BUFFER_RECT);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
  }
  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);

  {
    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer(
        {event_wait_list, num_events_in_wait_list}, return_event);
    if (!mux_command_buffer) {
      return CL_OUT_OF_RESOURCES;
    }

    auto device_index = command_queue->getDeviceIndex();
    auto mux_buffer =
        static_cast<cl_mem_buffer>(buffer)->mux_buffers[device_index];

    mux_buffer_region_info_t r_info{
        {region[0], region[1], region[2]},
        {buffer_origin[0], buffer_origin[1], buffer_origin[2]},
        {host_origin[0], host_origin[1], host_origin[2]},
        {buffer_row_pitch, buffer_slice_pitch},
        {host_row_pitch, host_slice_pitch},
    };

    auto mux_error = muxCommandReadBufferRegions(
        *mux_command_buffer, mux_buffer, ptr, &r_info, 1, 0, nullptr, nullptr);
    if (mux_error) {
      auto error = cl::getErrorFrom(mux_error);
      if (return_event) {
        return_event->complete(error);
      }
      return error;
    }

    cl::retainInternal(buffer);

    if (auto error =
            static_cast<cl_mem_buffer>(buffer)->synchronize(command_queue)) {
      return error;
    }

    if (auto error = command_queue->registerDispatchCallback(
            *mux_command_buffer, return_event,
            [buffer]() { cl::releaseInternal(buffer); })) {
      return error;
    }
  }

  if (blocking_read) {
    const cl_int ret = cl::WaitForEvents(1, &event_release_guard.get());
    if (CL_SUCCESS != ret) {
      return ret;
    }
  }

  if ((nullptr != event) && event_release_guard) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueCopyBufferRect(
    cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
    const size_t *src_origin, const size_t *dst_origin, const size_t *region,
    size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch,
    size_t dst_slice_pitch, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueCopyBufferRect");

  // Set pitch defaults if needed before we validate.
  if (src_row_pitch == 0) {
    src_row_pitch = region[0];
  }
  if (dst_row_pitch == 0) {
    dst_row_pitch = region[0];
  }
  if (src_slice_pitch == 0) {
    src_slice_pitch = region[1] * src_row_pitch;
  }
  if (dst_slice_pitch == 0) {
    dst_slice_pitch = region[1] * dst_row_pitch;
  }

  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  cl_int error = cl::validate::CopyBufferRectArguments(
      command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region,
      src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch);
  OCL_CHECK(error != CL_SUCCESS, return error);

  error = cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                      command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  cl_event return_event = nullptr;
  if (nullptr != event) {
    auto new_event =
        _cl_event::create(command_queue, CL_COMMAND_COPY_BUFFER_RECT);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
    *event = return_event;
  }

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  auto mux_command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  auto device_index = command_queue->getDeviceIndex();
  auto mux_src_buffer =
      static_cast<cl_mem_buffer>(src_buffer)->mux_buffers[device_index];
  auto mux_dst_buffer =
      static_cast<cl_mem_buffer>(dst_buffer)->mux_buffers[device_index];

  mux_buffer_region_info_t r_info{
      {region[0], region[1], region[2]},
      {src_origin[0], src_origin[1], src_origin[2]},
      {dst_origin[0], dst_origin[1], dst_origin[2]},
      {src_row_pitch, src_slice_pitch},
      {dst_row_pitch, dst_slice_pitch},
  };

  const mux_result_t mux_error = muxCommandCopyBufferRegions(
      *mux_command_buffer, mux_src_buffer, mux_dst_buffer, &r_info, 1, 0,
      nullptr, nullptr);

  if (mux_success != mux_error) {
    auto error = cl::getErrorFrom(mux_error);
    if (return_event) {
      return_event->complete(error);
    }
    return error;
  }

  cl::retainInternal(src_buffer);
  cl::retainInternal(dst_buffer);

  // TODO: Does dst_buffer need to be synchronized as well?
  if (auto error =
          static_cast<cl_mem_buffer>(src_buffer)->synchronize(command_queue)) {
    return error;
  }

  return command_queue->registerDispatchCallback(
      *mux_command_buffer, return_event, [src_buffer, dst_buffer]() {
        cl::releaseInternal(src_buffer);
        cl::releaseInternal(dst_buffer);
      });
}

CL_API_ENTRY void *CL_API_CALL cl::EnqueueMapBuffer(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map,
    cl_map_flags map_flags, size_t offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueMapBuffer");
  OCL_CHECK(!command_queue,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_COMMAND_QUEUE);
            return nullptr);
  OCL_CHECK(!buffer, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_MEM_OBJECT);
            return nullptr);
  OCL_CHECK(CL_MEM_OBJECT_BUFFER != buffer->type,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_MEM_OBJECT);
            return nullptr);

  OCL_CHECK(command_queue->context != buffer->context,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);

  OCL_CHECK(offset > buffer->size,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK((offset + size) > buffer->size,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(0 == size, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  if (cl_map_flags(0) == map_flags) {
    // https://cvs.khronos.org/bugzilla/show_bug.cgi?id=7390 states that a
    // map flag of zero is an implicit read and write mapping.
    map_flags = CL_MAP_READ | CL_MAP_WRITE;
  }

  const bool read_access = cl::validate::IsInBitSet(map_flags, CL_MAP_READ);
  const bool write_access = cl::validate::IsInBitSet(map_flags, CL_MAP_WRITE);
  const bool write_invalidate_region_access =
      cl::validate::IsInBitSet(map_flags, CL_MAP_WRITE_INVALIDATE_REGION);

  OCL_CHECK(map_flags &
                ~(CL_MAP_READ | CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK((read_access || write_access) && write_invalidate_region_access,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  cl_int error =
      cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                  command_queue->context, event, blocking_map);
  OCL_CHECK(error != CL_SUCCESS, OCL_SET_IF_NOT_NULL(errcode_ret, error);
            return nullptr);

  // The buffer offset is given in bytes and CL_DEVICE_MEM_BASE_ADDR_ALIGN is in
  // bits.
  OCL_CHECK(buffer->optional_parent &&
                (0 != (static_cast<cl_mem_buffer>(buffer)->offset %
                       (command_queue->device->mem_base_addr_align / 8))),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_MISALIGNED_SUB_BUFFER_OFFSET);
            return nullptr);

  OCL_CHECK(
      read_access &&
          (cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_WRITE_ONLY) ||
           cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_NO_ACCESS)),
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
      return nullptr);
  OCL_CHECK(
      (write_access || write_invalidate_region_access) &&
          (cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_READ_ONLY) ||
           cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_NO_ACCESS)),
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
      return nullptr);

  // Overlapping regions mapped for writing is not allowed.
  if ((write_access || write_invalidate_region_access) &&
      buffer->overlaps(offset, size)) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
    return nullptr;
  }

  cl_event map_completion_event = nullptr;
  if (blocking_map || (nullptr != event)) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_MAP_BUFFER);
    if (!new_event) {
      OCL_SET_IF_NOT_NULL(errcode_ret, new_event.error());
      return nullptr;
    }
    map_completion_event = *new_event;
  }
  cl::release_guard<cl_event> event_release_guard(map_completion_event,
                                                  cl::ref_count_type::EXTERNAL);

  if (auto error =
          static_cast<cl_mem_buffer>(buffer)->synchronize(command_queue)) {
    OCL_SET_IF_NOT_NULL(errcode_ret, error);
    return nullptr;
  }

  void *mappedBuffer = nullptr;
  error = buffer->pushMapMemory(
      command_queue, &mappedBuffer, offset, size, read_access, write_access,
      write_invalidate_region_access,
      {event_wait_list, num_events_in_wait_list}, event_release_guard.get());
  if (error) {
    OCL_SET_IF_NOT_NULL(errcode_ret, error);
    return nullptr;
  }

  if (blocking_map) {
    const cl_int ret = cl::WaitForEvents(1, &event_release_guard.get());
    if (CL_SUCCESS != ret) {
      OCL_SET_IF_NOT_NULL(errcode_ret, ret);
      return nullptr;
    }
  }

  if ((nullptr != event) && event_release_guard) {
    *event = event_release_guard.dismiss();
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return mappedBuffer;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::EnqueueWriteBuffer(cl_command_queue command_queue, cl_mem buffer,
                       cl_bool blocking_write, size_t offset, size_t size,
                       const void *ptr, cl_uint num_events_in_wait_list,
                       const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueWriteBuffer");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!buffer, return CL_INVALID_MEM_OBJECT);
  OCL_CHECK(!(command_queue->context), return CL_INVALID_CONTEXT);
  OCL_CHECK(!(buffer->context), return CL_INVALID_CONTEXT);
  OCL_CHECK((command_queue->context != buffer->context),
            return CL_INVALID_CONTEXT);
  OCL_CHECK(buffer->size < (offset + size), return CL_INVALID_VALUE);
  OCL_CHECK(!ptr, return CL_INVALID_VALUE);
  OCL_CHECK(0 == size, return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_READ_ONLY),
            return CL_INVALID_OPERATION);
  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_NO_ACCESS),
            return CL_INVALID_OPERATION);

  const cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event,
      blocking_write);
  OCL_CHECK(error != CL_SUCCESS, return error);

  cl_event return_event = nullptr;
  if (blocking_write || (nullptr != event)) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_WRITE_BUFFER);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
  }
  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);

  {
    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer(
        {event_wait_list, num_events_in_wait_list}, event_release_guard.get());
    if (!mux_command_buffer) {
      return CL_OUT_OF_RESOURCES;
    }

    auto device_index = command_queue->getDeviceIndex();
    auto mux_buffer =
        static_cast<cl_mem_buffer>(buffer)->mux_buffers[device_index];
    auto mux_error =
        muxCommandWriteBuffer(*mux_command_buffer, mux_buffer, offset, ptr,
                              size, 0, nullptr, nullptr);
    if (mux_error) {
      auto error = cl::getErrorFrom(mux_error);
      if (event_release_guard) {
        event_release_guard->complete(error);
      }
      return error;
    }

    cl::retainInternal(buffer);

    if (auto error =
            static_cast<cl_mem_buffer>(buffer)->synchronize(command_queue)) {
      return error;
    }

    if (auto error = command_queue->registerDispatchCallback(
            *mux_command_buffer, return_event,
            [buffer]() { cl::releaseInternal(buffer); })) {
      return error;
    }
  }

  if (blocking_write) {
    const cl_int ret = cl::WaitForEvents(1, &event_release_guard.get());
    if (CL_SUCCESS != ret) {
      return ret;
    }
  }

  if ((nullptr != event) && event_release_guard) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueReadBuffer(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
    size_t offset, size_t size, void *ptr, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueReadBuffer");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!buffer, return CL_INVALID_MEM_OBJECT);
  OCL_CHECK(!(command_queue->context), return CL_INVALID_CONTEXT);
  OCL_CHECK(!(buffer->context), return CL_INVALID_CONTEXT);
  OCL_CHECK((command_queue->context != buffer->context),
            return CL_INVALID_CONTEXT);
  OCL_CHECK(buffer->size < (offset + size), return CL_INVALID_VALUE);
  OCL_CHECK(!ptr, return CL_INVALID_VALUE);
  OCL_CHECK(0 == size, return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_WRITE_ONLY),
            return CL_INVALID_OPERATION);
  OCL_CHECK(cl::validate::IsInBitSet(buffer->flags, CL_MEM_HOST_NO_ACCESS),
            return CL_INVALID_OPERATION);

  const cl_int error =
      cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                  command_queue->context, event, blocking_read);
  OCL_CHECK(error != CL_SUCCESS, return error);

  cl_event return_event = nullptr;
  if (blocking_read || (nullptr != event)) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_READ_BUFFER);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
  }
  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);

  {
    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer(
        {event_wait_list, num_events_in_wait_list}, event_release_guard.get());
    if (!mux_command_buffer) {
      return CL_OUT_OF_RESOURCES;
    }

    auto device_index = command_queue->getDeviceIndex();
    auto mux_buffer =
        static_cast<cl_mem_buffer>(buffer)->mux_buffers[device_index];
    auto mux_error =
        muxCommandReadBuffer(*mux_command_buffer, mux_buffer, offset, ptr, size,
                             0, nullptr, nullptr);
    if (mux_error) {
      return cl::getErrorFrom(mux_error);
    }

    cl::retainInternal(buffer);

    if (auto error =
            static_cast<cl_mem_buffer>(buffer)->synchronize(command_queue)) {
      return error;
    }

    if (auto error = command_queue->registerDispatchCallback(
            *mux_command_buffer, return_event,
            [buffer]() { cl::releaseInternal(buffer); })) {
      return error;
    }
  }

  if (blocking_read) {
    const cl_int ret = cl::WaitForEvents(1, &event_release_guard.get());
    if (CL_SUCCESS != ret) {
      return ret;
    }
  }

  if ((nullptr != event) && event_release_guard) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::EnqueueCopyBuffer(cl_command_queue command_queue, cl_mem src_buffer,
                      cl_mem dst_buffer, size_t src_offset, size_t dst_offset,
                      size_t size, cl_uint num_events_in_wait_list,
                      const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueCopyBuffer");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);

  cl_int error = cl::validate::CopyBufferArguments(
      command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size);
  OCL_CHECK(error != CL_SUCCESS, return error);

  OCL_CHECK(!event_wait_list && num_events_in_wait_list > 0,
            return CL_INVALID_EVENT_WAIT_LIST);
  OCL_CHECK(event_wait_list && num_events_in_wait_list == 0,
            return CL_INVALID_EVENT_WAIT_LIST);

  error = cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                      command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  cl_event return_event = nullptr;
  if (nullptr != event) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_COPY_BUFFER);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
    *event = return_event;
  }

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  auto mux_command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  auto device_index = command_queue->getDeviceIndex();
  auto mux_src_buffer =
      static_cast<cl_mem_buffer>(src_buffer)->mux_buffers[device_index];
  auto mux_dst_buffer =
      static_cast<cl_mem_buffer>(dst_buffer)->mux_buffers[device_index];
  auto mux_error = muxCommandCopyBuffer(*mux_command_buffer, mux_src_buffer,
                                        src_offset, mux_dst_buffer, dst_offset,
                                        size, 0, nullptr, nullptr);
  if (mux_error) {
    auto error = cl::getErrorFrom(mux_error);
    if (return_event) {
      return_event->complete(error);
    }
    return error;
  }

  cl::retainInternal(src_buffer);
  cl::retainInternal(dst_buffer);

  // TODO: Does dst_buffer need to be synchronized as well?
  if (auto error =
          static_cast<cl_mem_buffer>(src_buffer)->synchronize(command_queue)) {
    return error;
  }

  return command_queue->registerDispatchCallback(
      *mux_command_buffer, return_event, [src_buffer, dst_buffer]() {
        cl::releaseInternal(src_buffer);
        cl::releaseInternal(dst_buffer);
      });
}

CL_API_ENTRY cl_int CL_API_CALL
cl::EnqueueFillBuffer(cl_command_queue command_queue, cl_mem buffer,
                      const void *pattern, size_t pattern_size, size_t offset,
                      size_t size, cl_uint num_events_in_wait_list,
                      const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueFillBuffer");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  cl_int error = cl::validate::FillBufferArguments(
      command_queue, buffer, pattern, pattern_size, offset, size);
  OCL_CHECK(error != CL_SUCCESS, return error);

  error = cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                      command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  cl_event return_event = nullptr;
  if (nullptr != event) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_FILL_BUFFER);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
    *event = return_event;
  }

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  auto mux_command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  auto device_index = command_queue->getDeviceIndex();
  auto mux_buffer =
      static_cast<cl_mem_buffer>(buffer)->mux_buffers[device_index];
  auto mux_error =
      muxCommandFillBuffer(*mux_command_buffer, mux_buffer, offset, size,
                           pattern, pattern_size, 0, nullptr, nullptr);
  if (mux_error) {
    auto error = cl::getErrorFrom(mux_error);
    if (return_event) {
      return_event->complete(error);
    }
    return error;
  }

  cl::retainInternal(buffer);

  if (auto error =
          static_cast<cl_mem_buffer>(buffer)->synchronize(command_queue)) {
    return error;
  }

  return command_queue->registerDispatchCallback(
      *mux_command_buffer, return_event,
      [buffer]() { cl::releaseInternal(buffer); });
}
