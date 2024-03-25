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

#include "ur/queue.h"

#include <algorithm>
#include <iterator>

#include "cargo/dynamic_array.h"
#include "ur/context.h"
#include "ur/event.h"
#include "ur/kernel.h"
#include "ur/memory.h"
#include "ur/mux.h"
#include "ur/platform.h"

namespace {
inline uint64_t getUSMOffset(const void *ptr, ur::allocation_info *usm_alloc) {
  auto offset = reinterpret_cast<uintptr_t>(ptr) -
                reinterpret_cast<uintptr_t>(usm_alloc->base_ptr);
  return static_cast<uint64_t>(offset);
}
}  // namespace

ur_queue_handle_t_::~ur_queue_handle_t_() {
  cleanupCompletedCommandBuffers();
  // Destroy command buffers that didn't get destroyed due to overflowing the
  // cache.
  while (true) {
    auto command_buffer = cached_command_buffers.dequeue();
    if (command_buffer.error() != cargo::result::success) {
      break;
    }
    muxDestroyCommandBuffer(device->mux_device, *command_buffer,
                            device->platform->mux_allocator_info);
  }
}

cargo::expected<ur_queue_handle_t, ur_result_t> ur_queue_handle_t_::create(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    ur_queue_flags_t flags) {
  mux_queue_t mux_queue;
  if (auto error = muxGetQueue(hDevice->mux_device, mux_queue_type_compute, 0,
                               &mux_queue)) {
    return cargo::make_unexpected(ur::resultFromMux(error));
  }
  auto queue =
      std::make_unique<ur_queue_handle_t_>(hContext, hDevice, flags, mux_queue);
  if (!queue) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }
  return queue.release();
}

void ur_queue_handle_t_::destroyCommandBuffer(
    mux_command_buffer_t command_buffer) {
  if (mux_success != muxResetCommandBuffer(command_buffer)) {
    // Command buffer reset failed, destroy it.
    muxDestroyCommandBuffer(device->mux_device, command_buffer,
                            device->platform->mux_allocator_info);
  }

  // Try and cache the command buffer first
  if (cargo::result::success !=
      cached_command_buffers.enqueue(command_buffer)) {
    // Then if we have no room to cache it, destroy it.
    muxDestroyCommandBuffer(device->mux_device, command_buffer,
                            device->platform->mux_allocator_info);
  }
}

cargo::expected<mux_command_buffer_t, ur_result_t>
ur_queue_handle_t_::getCommandBuffer(const ur_event_handle_t signal_event,
                                     uint32_t num_wait_events,
                                     const ur_event_handle_t *wait_events) {
  mux_command_buffer_t command_buffer;
  if (auto cached_command_buffer = cached_command_buffers.dequeue()) {
    // We have a cached command buffer we can use.
    command_buffer = *cached_command_buffer;
  } else {
    // Otherwise create a new command buffer.
    auto err = muxCreateCommandBuffer(device->mux_device, nullptr,
                                      device->platform->mux_allocator_info,
                                      &command_buffer);
    if (err != mux_success) {
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }
  }

  cargo::small_vector<mux_semaphore_t, 8> wait_semaphores;

  // We wait on the last pending dispatch (if there is one).
  if (!pending_dispatches.empty()) {
    if (wait_semaphores.push_back(
            pending_dispatches.back().signal_event->mux_semaphore)) {
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }
  }

  // Wait for running dispatches.
  for (auto &running_dispatch : running_dispatches) {
    if (wait_semaphores.push_back(
            running_dispatch.signal_event->mux_semaphore)) {
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }
  }

  // Wait for user event semaphores.
  for (uint32_t waitEventIndex = 0; waitEventIndex < num_wait_events;
       waitEventIndex++) {
    if (wait_semaphores.push_back(wait_events[waitEventIndex]->mux_semaphore)) {
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }
  }

  // It's probably worth taking the time to remove duplicates.
  wait_semaphores.erase(
      std::unique(wait_semaphores.begin(), wait_semaphores.end()),
      wait_semaphores.end());

  // Add our new dispatch to the list.
  if (pending_dispatches.push_back(
          {command_buffer, std::move(wait_semaphores), signal_event})) {
    destroyCommandBuffer(command_buffer);
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_RESOURCES);
  }

  return command_buffer;
}

ur_result_t ur_queue_handle_t_::wait() {
  auto error = flush();
  if (error != UR_RESULT_SUCCESS) {
    return error;
  }
  if (auto error = muxWaitAll(mux_queue)) {
    return ur::resultFromMux(error);
  }
  cleanupCompletedCommandBuffers();
  return UR_RESULT_SUCCESS;
}

ur_result_t ur_queue_handle_t_::flush() {
  if (auto error = cleanupCompletedCommandBuffers()) {
    return error;
  }
  const std::lock_guard<std::mutex> lock(mutex);
  for (auto &dispatch : pending_dispatches) {
    auto mux_result = muxFinalizeCommandBuffer(dispatch.command_buffer);
    if (mux_result != mux_success) {
      return UR_RESULT_ERROR_INVALID_QUEUE;
    }

    mux_semaphore_t *wait_semaphores = dispatch.wait_semaphores.size()
                                           ? dispatch.wait_semaphores.data()
                                           : nullptr;

    if (muxDispatch(mux_queue, dispatch.command_buffer,
                    dispatch.signal_event->mux_fence, wait_semaphores,
                    dispatch.wait_semaphores.size(),
                    &dispatch.signal_event->mux_semaphore, 1, nullptr,
                    nullptr) != mux_success) {
      return UR_RESULT_ERROR_OUT_OF_RESOURCES;
    }

    running_dispatches.push_back({dispatch.command_buffer,
                                  std::move(dispatch.wait_semaphores),
                                  dispatch.signal_event});
  }

  pending_dispatches.clear();
  return UR_RESULT_SUCCESS;
}

ur_result_t ur_queue_handle_t_::cleanupCompletedCommandBuffers() {
  // Check to see if there are any command buffers ready to be cleaned up.
  while (true) {
    const std::lock_guard<std::mutex> lock(mutex);

    if (running_dispatches.empty()) {
      // There are no running command buffers so we can stop processing.
      break;
    }

    // Check if the first running command buffer has completed.
    auto fence = running_dispatches.front().signal_event->mux_fence;
    const mux_result_t error = muxTryWait(mux_queue, 0, fence);
    if (error != mux_success && error != mux_fence_not_ready &&
        error != mux_error_fence_failure) {
      return UR_RESULT_ERROR_INVALID_QUEUE;
    }

    if (mux_fence_not_ready == error) {
      // The command buffer wasn't yet complete. Because of how our command
      // buffers are linearly chained together (we have an in order queue)
      // we can bail now as if this command buffer isn't complete, future
      // ones will not have completed yet either.
      return UR_RESULT_SUCCESS;
    }

    // Note that by this point 'error' may be either mux_success or
    // mux_error_fence_failure.  This function does not care about
    // the difference (the error is handled elsewhere), we just consider
    // either case to mean that the group is 'complete' and process it
    // accordingly.

    // The command buffer has completed so stop tracking it then destroy it.
    auto completed = std::move(running_dispatches.front());
    running_dispatches.pop_front();
    destroyCommandBuffer(completed.command_buffer);

    // Remove the signal semaphore from pending dispatches.
    for (auto &dispatch : pending_dispatches) {
      auto &wait_semaphores = dispatch.wait_semaphores;
      auto found = std::find(wait_semaphores.begin(), wait_semaphores.end(),
                             completed.signal_event->mux_semaphore);
      if (found != wait_semaphores.end()) {
        wait_semaphores.erase(found);
      }
    }

    // Append the completed event to the cleanup list.
    if (completed_events.push_back(completed.signal_event)) {
      return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Get list of all wait_semaphores from running dispatches.
    cargo::small_vector<mux_semaphore_t, 64> running_wait_semaphores;
    for (auto &running : running_dispatches) {
      if (!running_wait_semaphores.insert(running_wait_semaphores.end(),
                                          running.wait_semaphores.begin(),
                                          running.wait_semaphores.end())) {
        return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    // Remove any duplicates from the list of all running wait semaphores.
    running_wait_semaphores.erase(std::unique(running_wait_semaphores.begin(),
                                              running_wait_semaphores.end()),
                                  running_wait_semaphores.end());

    // Iterate over completed events and mark any which are no
    // longer being waited upon by running dispatches for release.
    cargo::small_vector<ur_event_handle_t, 16> dead_events;
    auto isWaitedUpon =
        [&running_wait_semaphores](mux_semaphore_t signal_semaphore) {
          return std::find(running_wait_semaphores.begin(),
                           running_wait_semaphores.end(),
                           signal_semaphore) != running_wait_semaphores.end();
        };

    for (auto signal_event : completed_events) {
      if (isWaitedUpon(signal_event->mux_semaphore)) {
        continue;
      }
      if (dead_events.push_back(signal_event)) {
        return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
      }
    }

    // Move discarded events to the back, then erase them.
    auto first_dead_event = std::stable_partition(
        completed_events.begin(), completed_events.end(),
        [&dead_events](ur_event_handle_t event) {
          // When semaphore is not in destroyed_semaphores return true which
          // moves it to the front, otherwise move it to the back.
          return std::find(dead_events.begin(), dead_events.end(), event) ==
                 dead_events.end();
        });
    completed_events.erase(first_dead_event, completed_events.end());

    for (auto &event : dead_events) {
      ur::release(event);
    }
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL
urQueueCreate(ur_context_handle_t hContext, ur_device_handle_t hDevice,
              const ur_queue_property_t *props, ur_queue_handle_t *phQueue) {
  if (!hContext || !hDevice) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (hContext->platform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }

  ur_queue_flags_t flags = 0;
  if (props) {
    while (0 != *props) {
      switch (*props) {
        case UR_QUEUE_PROPERTIES_FLAGS: {
          const auto read_flags = *(++props);
          if (0xf < read_flags) {
            return UR_RESULT_ERROR_INVALID_ENUMERATION;
          }
          flags = read_flags;
        } break;
        default:
          // TODO: Handle properties other than flags (see CA-4710).
          return UR_RESULT_ERROR_INVALID_ENUMERATION;
      }
      ++props;
    }
  }

  if (!phQueue) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }
  auto queue = ur_queue_handle_t_::create(hContext, hDevice, flags);
  if (!queue) {
    return queue.error();
  }
  *phQueue = *queue;
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urQueueRetain(ur_queue_handle_t hQueue) {
  if (!hQueue) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::retain(hQueue);
}

UR_APIEXPORT ur_result_t UR_APICALL urQueueRelease(ur_queue_handle_t hQueue) {
  if (!hQueue) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::release(hQueue);
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueMemBufferWrite(
    ur_queue_handle_t hQueue, ur_mem_handle_t hBuffer, bool blockingWrite,
    size_t offset, size_t size, const void *src, uint32_t numEventsInWaitList,
    const ur_event_handle_t *eventWaitList, ur_event_handle_t *pEvent) {
  if (!hQueue || !hBuffer) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (hQueue->context && hQueue->context->platform &&
      hQueue->context->platform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }
  if (hBuffer->type != UR_MEM_TYPE_BUFFER) {
    return UR_RESULT_ERROR_INVALID_MEM_OBJECT;
  }
  // TODO: offset bounds checking
  (void)offset;
  // TODO: size bounds checking
  (void)size;
  if (!src) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (numEventsInWaitList && !eventWaitList) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  // Synchronize the state of the memory buffer across devices in the context.
  if (const auto error = hBuffer->sync(hQueue)) {
    return error;
  }

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto error = muxCommandWriteBuffer(
            *command_buffer_or_err, hBuffer->buffers[0].mux_buffer, offset, src,
            size, 0, nullptr, nullptr)) {
      return ur::resultFromMux(error);
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  if (blockingWrite) {
    hQueue->wait();
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueMemBufferRead(
    ur_queue_handle_t hQueue, ur_mem_handle_t hBuffer, bool blockingRead,
    size_t offset, size_t size, void *dst, uint32_t numEventsInWaitList,
    const ur_event_handle_t *eventWaitList, ur_event_handle_t *pEvent) {
  if (!hQueue || !hBuffer) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (hQueue->context && hQueue->context->platform &&
      hQueue->context->platform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }
  if (hBuffer->type != UR_MEM_TYPE_BUFFER) {
    return UR_RESULT_ERROR_INVALID_MEM_OBJECT;
  }
  // TODO: offset bounds checking
  (void)offset;
  // TODO: size bounds checking
  (void)size;
  if (!dst) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  // Synchronize the state of the memory buffer across devices in the context.
  if (const auto error = hBuffer->sync(hQueue)) {
    return error;
  }

  const auto mux_device_idx = hQueue->getDeviceIdx();
  const auto mux_buffer = hBuffer->buffers[mux_device_idx].mux_buffer;

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto error =
            muxCommandReadBuffer(*command_buffer_or_err, mux_buffer, offset,
                                 dst, size, 0, nullptr, nullptr)) {
      return ur::resultFromMux(error);
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  if (blockingRead) {
    hQueue->wait();
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueKernelLaunch(
    ur_queue_handle_t hQueue, ur_kernel_handle_t hKernel, uint32_t workDim,
    const size_t *globalWorkOffset, const size_t *globalWorkSize,
    const size_t *localWorkSize, uint32_t numEventsInWaitList,
    const ur_event_handle_t *eventWaitList, ur_event_handle_t *pEvent) {
  if (!hKernel || !hQueue) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!globalWorkOffset || !globalWorkSize) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  // TODO: The user is allowed to pass a null local size, in which case the
  // implementation is required to provide one.
  assert(
      localWorkSize &&
      "Error: Local size for urEnqueueKernelLaunch is currently non-optional");

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  const auto num_args = hKernel->arguments.size();
  mux_ndrange_options_t options;
  cargo::dynamic_array<mux_descriptor_info_t> descriptors;
  if (descriptors.alloc(num_args)) {
    return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  }

  const auto device_idx = hQueue->getDeviceIdx();

  for (unsigned arg_idx = 0; arg_idx < num_args; ++arg_idx) {
    const auto buffer = hKernel->arguments[arg_idx].mem_handle;
    if (buffer) {
      // Synchronize the state of the memory buffer across devices in the
      // context.
      if (const auto error = buffer->sync(hQueue)) {
        return error;
      }

      descriptors[arg_idx].type = mux_descriptor_info_type_buffer;
      descriptors[arg_idx].buffer_descriptor.buffer =
          buffer->buffers[device_idx].mux_buffer;
      descriptors[arg_idx].buffer_descriptor.offset = 0;
    } else {
      descriptors[arg_idx].type = mux_descriptor_info_type_plain_old_data;
      descriptors[arg_idx].plain_old_data_descriptor.data =
          hKernel->arguments[arg_idx].value.data;
      descriptors[arg_idx].plain_old_data_descriptor.length =
          hKernel->arguments[arg_idx].value.size;
    }
  }
  options.descriptors = descriptors.data();
  options.descriptors_length = num_args;
  std::fill_n(std::begin(options.local_size),
              sizeof(options.local_size) / sizeof(options.local_size[0]), 1);
  std::copy_n(localWorkSize, workDim, std::begin(options.local_size));
  options.global_offset = globalWorkOffset;
  options.global_size = globalWorkSize;
  options.dimensions = workDim;

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }
    if (auto error = muxCommandNDRange(
            *command_buffer_or_err, hKernel->device_kernel_map[hQueue->device],
            options, 0, nullptr, nullptr)) {
      return ur::resultFromMux(error);
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueMemBufferCopy(
    ur_queue_handle_t hQueue, ur_mem_handle_t hBufferSrc,
    ur_mem_handle_t hBufferDst, size_t srcOffset, size_t dstOffset, size_t size,
    uint32_t numEventsInWaitList, const ur_event_handle_t *eventWaitList,
    ur_event_handle_t *pEvent) {
  if (!hQueue || !hBufferSrc || !hBufferDst) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  // Synchronize the state of the memory buffer across devices in the context.
  if (const auto error = hBufferSrc->sync(hQueue)) {
    return error;
  }

  // The destination buffer still needs to be synchronized even though it's
  // about to be written into. This is because the sync function is responsible
  // for updating the last command buffer which touched this buffer, so
  // subsequent commands on different queues will know to sync the state of
  // device specific buffers after this command has executed.
  if (const auto error = hBufferDst->sync(hQueue)) {
    return error;
  }

  const auto device_idx = hQueue->getDeviceIdx();
  const auto mux_src_buffer = hBufferSrc->buffers[device_idx].mux_buffer;
  const auto mux_dst_buffer = hBufferDst->buffers[device_idx].mux_buffer;

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto error = muxCommandCopyBuffer(
            *command_buffer_or_err, mux_src_buffer, srcOffset, mux_dst_buffer,
            dstOffset, size, 0, nullptr, nullptr)) {
      return ur::resultFromMux(error);
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueMemBufferFill(
    ur_queue_handle_t hQueue, ur_mem_handle_t hBuffer, const void *pattern,
    size_t patternSize, size_t offset, size_t size,
    uint32_t numEventsInWaitList, const ur_event_handle_t *eventWaitList,
    ur_event_handle_t *pEvent) {
  if (!hQueue || !hBuffer) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!pattern) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  // Synchronize the state of the memory buffer across devices in the context.
  if (const auto error = hBuffer->sync(hQueue)) {
    return error;
  }

  const auto device_idx = hQueue->getDeviceIdx();
  const auto mux_buffer = hBuffer->buffers[device_idx].mux_buffer;

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto error = muxCommandFillBuffer(*command_buffer_or_err, mux_buffer,
                                          offset, size, pattern, patternSize, 0,
                                          nullptr, nullptr)) {
      return ur::resultFromMux(error);
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueMemBufferMap(
    ur_queue_handle_t hQueue, ur_mem_handle_t hBuffer, bool blockingMap,
    ur_map_flags_t mapFlags, size_t offset, size_t size,
    uint32_t numEventsInWaitList, const ur_event_handle_t *eventWaitList,
    ur_event_handle_t *pEvent, void **retMap) {
  if (!hQueue || !hBuffer) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (0x3 < mapFlags) {
    return UR_RESULT_ERROR_INVALID_ENUMERATION;
  }

  if (!retMap) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  // Synchronize the state of the memory buffer across devices in the context.
  if (const auto error = hBuffer->sync(hQueue)) {
    return error;
  }

  const auto mux_device = hQueue->device->mux_device;
  const auto mux_memory = hBuffer->buffers[hQueue->getDeviceIdx()].mux_memory;

  {
    const std::lock_guard<std::mutex> lock(hBuffer->mutex);
    // First we check if there are already any active mappings to this buffer.
    if (!hBuffer->map_count) {
      // If there is no active mapping then call into mux.
      // Here we always map the entire buffer at an offset of zero, meaning any
      // subsequent mappings on this buffer can reuse this map.
      if (const auto error =
              muxMapMemory(mux_device, mux_memory, 0, hBuffer->size,
                           &hBuffer->host_base_ptr)) {
        return ur::resultFromMux(error);
      }
    }
    // Record this mapping.
    hBuffer->map_count++;
    // Then set the mapped pointer to the base pointer adjusted to the offset.
    *retMap = static_cast<char *>(hBuffer->host_base_ptr) + offset;

    // Only write commands will need to flush to device (read maps don't affect
    // on device memory when they are unmapped).
    if (mapFlags & UR_MAP_FLAG_WRITE) {
      hBuffer->write_mapping_states[*retMap] = {offset, size};
    }
  }

  // mapping state to pass to the user callback
  struct mapping_state_t {
    mapping_state_t(ur_mem_handle_t mem, size_t offset, size_t size,
                    uint32_t device_index)
        : mem(mem), offset(offset), size(size), device_index(device_index) {}

    void flushMemoryFromDevice() {
      mux_device_t device = mem->context->devices[device_index]->mux_device;
      mux_memory_t memory = mem->buffers[device_index].mux_memory;
      const mux_result_t error =
          muxFlushMappedMemoryFromDevice(device, memory, offset, size);
      assert(mux_success == error && "muxFlushMappedMemoryFromDevice failed!");
      (void)error;
    }

    ur_mem_handle_t mem;
    size_t offset;
    size_t size;
    uint32_t device_index;
  } *mapping =
      new mapping_state_t(hBuffer, offset, size, hQueue->getDeviceIdx());

  auto callback = [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
    auto mapping = static_cast<mapping_state_t *>(user_data);
    std::lock_guard<std::mutex> const lock(mapping->mem->mutex);
    mapping->flushMemoryFromDevice();
    delete mapping;
  };

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto mux_error = muxCommandUserCallback(
            *command_buffer_or_err, callback, mapping, 0, nullptr, nullptr)) {
      return ur::resultFromMux(mux_error);
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  if (blockingMap) {
    hQueue->wait();
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueMemUnmap(
    ur_queue_handle_t hQueue, ur_mem_handle_t hMem, void *mappedPtr,
    uint32_t numEventsInWaitList, const ur_event_handle_t *eventWaitList,
    ur_event_handle_t *pEvent) {
  if (!hQueue || !hMem) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!mappedPtr) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  // mapping state to pass to the user callback
  struct unmapping_state_t {
    unmapping_state_t(ur_mem_handle_t mem, void *mapped_ptr,
                      uint32_t device_index)
        : mem(mem), mapped_ptr(mapped_ptr), device_index(device_index) {}

    void unMapMemory() {
      const auto mux_device = mem->context->devices[device_index]->mux_device;
      const auto mux_memory = mem->buffers[device_index].mux_memory;
      // We only need to flush the write maps to device since reads don't have
      // any side effects.
      const auto &write_mapping_states = mem->write_mapping_states;
      auto it = write_mapping_states.find(mapped_ptr);
      if (std::end(write_mapping_states) != it) {
        const auto &map_state = it->second;
        auto error = muxFlushMappedMemoryToDevice(
            mux_device, mux_memory, map_state.map_offset, map_state.map_size);
        assert(error == mux_success &&
               "muxFlushMappedMemoryToDevice to device failed!\n");
        (void)error;
      }

      // Only the last mapping on this buffer needs to call into mux to unmap
      // it.
      if (1 == mem->map_count) {
        auto error = muxUnmapMemory(mux_device, mux_memory);
        assert(error == mux_success && "muxUnmapMemory to device failed!\n");
        (void)error;
      }

      mem->map_count--;
    }

    ur_mem_handle_t mem;
    void *mapped_ptr;
    uint32_t device_index;
  } *unmapping = new unmapping_state_t(hMem, mappedPtr, hQueue->getDeviceIdx());

  auto callback = [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
    auto unmapping = static_cast<unmapping_state_t *>(user_data);
    std::lock_guard<std::mutex> const lock(unmapping->mem->mutex);
    unmapping->unMapMemory();
    delete unmapping;
  };

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto mux_error = muxCommandUserCallback(
            *command_buffer_or_err, callback, unmapping, 0, nullptr, nullptr)) {
      return ur::resultFromMux(mux_error);
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueMemBufferReadRect(
    ur_queue_handle_t hQueue, ur_mem_handle_t hBuffer, bool blockingRead,
    ur_rect_offset_t bufferOffset, ur_rect_offset_t hostOffset,
    ur_rect_region_t region, size_t bufferRowPitch, size_t bufferSlicePitch,
    size_t hostRowPitch, size_t hostSlicePitch, void *dst,
    uint32_t numEventsInWaitList, const ur_event_handle_t *eventWaitList,
    ur_event_handle_t *pEvent) {
  if (!hQueue || !hBuffer) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!dst) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  // Synchronize the state of the memory buffer across devices in the context.
  if (const auto error = hBuffer->sync(hQueue)) {
    return error;
  }

  mux_buffer_region_info_t mux_region{
      {static_cast<size_t>(region.width), static_cast<size_t>(region.height),
       static_cast<size_t>(region.depth)},
      {static_cast<size_t>(bufferOffset.x), static_cast<size_t>(bufferOffset.y),
       static_cast<size_t>(bufferOffset.z)},
      {static_cast<size_t>(hostOffset.x), static_cast<size_t>(hostOffset.y),
       static_cast<size_t>(hostOffset.z)},
      {bufferRowPitch, bufferSlicePitch},
      {hostRowPitch, hostSlicePitch}};

  const auto device_idx = hQueue->getDeviceIdx();
  const auto mux_buffer = hBuffer->buffers[device_idx].mux_buffer;

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto error =
            muxCommandReadBufferRegions(*command_buffer_or_err, mux_buffer, dst,
                                        &mux_region, 1, 0, nullptr, nullptr)) {
      return ur::resultFromMux(error);
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  if (blockingRead) {
    hQueue->wait();
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueMemBufferWriteRect(
    ur_queue_handle_t hQueue, ur_mem_handle_t hBuffer, bool blockingWrite,
    ur_rect_offset_t bufferOffset, ur_rect_offset_t hostOffset,
    ur_rect_region_t region, size_t bufferRowPitch, size_t bufferSlicePitch,
    size_t hostRowPitch, size_t hostSlicePitch, void *src,
    uint32_t numEventsInWaitList, const ur_event_handle_t *eventWaitList,
    ur_event_handle_t *pEvent) {
  if (!hQueue || !hBuffer) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!src) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  // Synchronize the state of the memory buffer across devices in the context.
  if (const auto error = hBuffer->sync(hQueue)) {
    return error;
  }

  mux_buffer_region_info_t mux_region{
      {static_cast<size_t>(region.width), static_cast<size_t>(region.height),
       static_cast<size_t>(region.depth)},
      {static_cast<size_t>(bufferOffset.x), static_cast<size_t>(bufferOffset.y),
       static_cast<size_t>(bufferOffset.z)},
      {static_cast<size_t>(hostOffset.x), static_cast<size_t>(hostOffset.y),
       static_cast<size_t>(hostOffset.z)},
      {bufferRowPitch, bufferSlicePitch},
      {hostRowPitch, hostSlicePitch}};

  const auto device_idx = hQueue->getDeviceIdx();
  const auto mux_buffer = hBuffer->buffers[device_idx].mux_buffer;

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto error = muxCommandWriteBufferRegions(*command_buffer_or_err,
                                                  mux_buffer, src, &mux_region,
                                                  1, 0, nullptr, nullptr)) {
      return ur::resultFromMux(error);
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  if (blockingWrite) {
    hQueue->wait();
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueMemBufferCopyRect(
    ur_queue_handle_t hQueue, ur_mem_handle_t hBufferSrc,
    ur_mem_handle_t hBufferDst, ur_rect_offset_t srcOrigin,
    ur_rect_offset_t dstOrigin, ur_rect_region_t srcRegion, size_t srcRowPitch,
    size_t srcSlicePitch, size_t dstRowPitch, size_t dstSlicePitch,
    uint32_t numEventsInWaitList, const ur_event_handle_t *eventWaitList,
    ur_event_handle_t *pEvent) {
  if (!hQueue || !hBufferSrc || !hBufferDst) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  // Synchronize the state of the memory buffer across devices in the context.
  if (const auto error = hBufferSrc->sync(hQueue)) {
    return error;
  }

  // The destination buffer still needs to be synchronized even though it's
  // about to be written into. This is because the sync function is responsible
  // for updating the last command buffer which touched this buffer, so
  // subsequent commands on different queues will know to sync the state of
  // device specific buffers after this command has executed.
  if (const auto error = hBufferDst->sync(hQueue)) {
    return error;
  }

  mux_buffer_region_info_t mux_region{
      {static_cast<size_t>(srcRegion.width),
       static_cast<size_t>(srcRegion.height),
       static_cast<size_t>(srcRegion.depth)},
      {static_cast<size_t>(srcOrigin.x), static_cast<size_t>(srcOrigin.y),
       static_cast<size_t>(srcOrigin.z)},
      {static_cast<size_t>(dstOrigin.x), static_cast<size_t>(dstOrigin.y),
       static_cast<size_t>(dstOrigin.z)},
      {srcRowPitch, srcSlicePitch},
      {dstRowPitch, dstSlicePitch}};

  const auto device_idx = hQueue->getDeviceIdx();
  const auto mux_src_buffer = hBufferSrc->buffers[device_idx].mux_buffer;
  const auto mux_dst_buffer = hBufferDst->buffers[device_idx].mux_buffer;

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto error = muxCommandCopyBufferRegions(
            *command_buffer_or_err, mux_src_buffer, mux_dst_buffer, &mux_region,
            1, 0, nullptr, nullptr)) {
      return ur::resultFromMux(error);
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueEventsWait(
    ur_queue_handle_t hQueue, uint32_t numEventsInWaitList,
    const ur_event_handle_t *eventWaitList, ur_event_handle_t *pEvent) {
  if (hQueue == nullptr) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  // Skip this check for now since we are trying to get this removed from spec.
  // if (event == nullptr) {
  //   return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  // }

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  // Enqueue an empty command buffer to block on the provided events, this
  // ensures that the wait works even for events from other queues.
  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueEventsWaitWithBarrier(
    ur_queue_handle_t hQueue, uint32_t numEventsInWaitList,
    const ur_event_handle_t *eventWaitList, ur_event_handle_t *pEvent) {
  if (hQueue == nullptr) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  // We can handle this identically to EventsWait (see above).
  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }
  }

  if (pEvent) {
    ur::retain(*event);
    *pEvent = *event;
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueUSMFill(
    ur_queue_handle_t hQueue, void *ptr, size_t patternSize,
    const void *pPattern, size_t size, uint32_t numEventsInWaitList,
    const ur_event_handle_t *eventWaitList, ur_event_handle_t *phEvent) {
  if (!hQueue) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!ptr || !pPattern) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (!hQueue->context) {
    return UR_RESULT_ERROR_INVALID_QUEUE;
  }

  if (!eventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  auto usm_alloc = hQueue->context->findUSMAllocation(ptr);
  if (!usm_alloc) {
    return UR_RESULT_ERROR_INVALID_MEM_OBJECT;
  }

  auto mux_buffer = usm_alloc->getMuxBufferForDevice(hQueue->device);
  if (!mux_buffer) {
    return UR_RESULT_ERROR_INVALID_QUEUE;
  }

  const uint64_t offset = getUSMOffset(ptr, usm_alloc);

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, eventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto error = muxCommandFillBuffer(*command_buffer_or_err, mux_buffer,
                                          offset, size, pPattern, patternSize,
                                          0, nullptr, nullptr)) {
      return ur::resultFromMux(error);
    }
  }

  if (phEvent != nullptr) {
    ur::retain(*event);
    *phEvent = *event;
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urEnqueueUSMMemcpy(
    ur_queue_handle_t hQueue, bool blocking, void *pDst, const void *pSrc,
    size_t size, uint32_t numEventsInWaitList,
    const ur_event_handle_t *phEventWaitList, ur_event_handle_t *phEvent) {
  if (!hQueue) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!pDst) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (!pSrc) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (!hQueue->context) {
    return UR_RESULT_ERROR_INVALID_QUEUE;
  }

  if (!phEventWaitList && numEventsInWaitList != 0) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  auto dst_usm_alloc = hQueue->context->findUSMAllocation(pDst);
  auto src_usm_alloc = hQueue->context->findUSMAllocation(pSrc);
  if (!dst_usm_alloc || !src_usm_alloc) {
    return UR_RESULT_ERROR_INVALID_MEM_OBJECT;
  }

  auto dst_mux_buffer = dst_usm_alloc->getMuxBufferForDevice(hQueue->device);
  auto src_mux_buffer = src_usm_alloc->getMuxBufferForDevice(hQueue->device);
  if (!dst_mux_buffer | !src_mux_buffer) {
    return UR_RESULT_ERROR_INVALID_QUEUE;
  }

  const uint64_t dst_offset = getUSMOffset(pDst, dst_usm_alloc);
  const uint64_t src_offset = getUSMOffset(pSrc, src_usm_alloc);

  auto event = ur_event_handle_t_::create(hQueue);

  if (!event) {
    return event.error();
  }

  {
    const std::lock_guard<std::mutex> lock(hQueue->mutex);
    auto command_buffer_or_err =
        hQueue->getCommandBuffer(*event, numEventsInWaitList, phEventWaitList);
    if (!command_buffer_or_err) {
      return command_buffer_or_err.error();
    }

    if (auto error = muxCommandCopyBuffer(
            *command_buffer_or_err, src_mux_buffer, src_offset, dst_mux_buffer,
            dst_offset, size, 0, nullptr, nullptr)) {
      return ur::resultFromMux(error);
    }
  }

  if (phEvent != nullptr) {
    ur::retain(*event);
    *phEvent = *event;
  }

  if (blocking) {
    hQueue->wait();
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urQueueFlush(ur_queue_handle_t hQueue) {
  if (!hQueue) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return hQueue->flush();
}

UR_APIEXPORT ur_result_t UR_APICALL urQueueFinish(ur_queue_handle_t hQueue) {
  if (!hQueue) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return hQueue->wait();
}
