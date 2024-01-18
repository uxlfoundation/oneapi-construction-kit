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

#include <host/buffer.h>
#include <host/command_buffer.h>
#include <host/device.h>
#include <host/host.h>
#include <host/image.h>
#include <host/kernel.h>
#include <host/query_pool.h>
#include <host/queue.h>
#include <host/semaphore.h>
#include <host/thread_pool.h>
#include <mux/config.h>
#include <mux/mux.h>
#include <utils/system.h>

#ifdef HOST_IMAGE_SUPPORT
#include <libimg/host.h>
#endif

#include <cassert>
#include <cstring>
#include <memory>
#include <new>
#include <vector>

namespace {

/// Increasing the slice count helps when thread slicing happens, or if a
/// kernel exists early, which allows other threads to pickup the extra work.
constexpr size_t slice_multiplier = 1;

void threadPoolCleanup(void *const v_queue, void *const v_command_buffer,
                       void *const v_fence, size_t terminate) {
  auto queue = static_cast<host::queue_s *>(v_queue);
  auto command_buffer = static_cast<host::command_buffer_s *>(v_command_buffer);

  auto host_fence = static_cast<host::fence_s *>(v_fence);

  const auto result = terminate ? mux_error_fence_failure : mux_success;
  // Fences are optional and may be null.
  if (host_fence) {
    host_fence->result = result;
  }

  if (nullptr != command_buffer->user_function) {
    command_buffer->user_function(command_buffer, result,
                                  command_buffer->user_data);
  }

  // Acquire a lock on the queue's mutex.
  const std::lock_guard<std::mutex> lock(queue->mutex);

  for (auto signal_semaphore : command_buffer->signal_semaphores) {
    static_cast<host::semaphore_s *>(signal_semaphore)->signal(terminate);
  }

  // and resize the signal_semaphores array
  command_buffer->signal_semaphores.clear();
}

void commandReadBuffer(host::command_info_s *info) {
  host::command_info_read_buffer_s *const read = &(info->read_command);

  auto buffer = static_cast<host::buffer_s *>(read->buffer);

  std::memcpy(read->host_pointer,
              static_cast<uint8_t *>(buffer->data) + read->offset, read->size);
}

void commandWriteBuffer(host::command_info_s *info) {
  host::command_info_write_buffer_s *const write = &(info->write_command);

  auto buffer = static_cast<host::buffer_s *>(write->buffer);

  std::memcpy(static_cast<uint8_t *>(buffer->data) + write->offset,
              write->host_pointer, write->size);
}

void commandFillBuffer(host::command_info_s *info) {
  host::command_info_fill_buffer_s *const fill = &(info->fill_command);

  auto buffer = static_cast<host::buffer_s *>(fill->buffer);

  size_t size = fill->pattern_size;
  uint8_t *const start = static_cast<uint8_t *>(buffer->data) + fill->offset;
  uint8_t *current = start + size;
  uint8_t *const end = start + fill->size;

  std::memcpy(start, fill->pattern, size);

  while (current + size < end) {
    std::memcpy(current, start, size);
    current += size;
    size *= 2;
  }

  std::memcpy(current, start, static_cast<size_t>(end - current));
}

void commandCopyBuffer(host::command_info_s *info) {
  host::command_info_copy_buffer_s *const copy = &(info->copy_command);

  auto dst_buffer = static_cast<host::buffer_s *>(copy->dst_buffer);
  auto src_buffer = static_cast<host::buffer_s *>(copy->src_buffer);

  std::memcpy(static_cast<uint8_t *>(dst_buffer->data) + copy->dst_offset,
              static_cast<uint8_t *>(src_buffer->data) + copy->src_offset,
              copy->size);
}

void commandReadImage(host::command_info_s *info) {
#ifdef HOST_IMAGE_SUPPORT
  const host::command_info_read_image_s &read = info->read_image_command;

  auto image = static_cast<host::image_s *>(read.image);
  const size_t origin[3] = {read.offset.x, read.offset.y, read.offset.z};
  const size_t region[3] = {read.extent.x, read.extent.y, read.extent.z};
  const size_t row_pitch = static_cast<size_t>(read.row_size);
  const size_t slice_pitch = static_cast<size_t>(read.slice_size);
  uint8_t *pointer = static_cast<uint8_t *>(read.pointer);

  libimg::HostReadImage(&image->image, origin, region, row_pitch, slice_pitch,
                        pointer);
#else
  (void)info;
#endif
}

void commandWriteImage(host::command_info_s *info) {
#ifdef HOST_IMAGE_SUPPORT
  const host::command_info_write_image_s &write = info->write_image_command;

  auto image = static_cast<host::image_s *>(write.image);
  size_t origin[3] = {write.offset.x, write.offset.y, write.offset.z};
  size_t region[3] = {write.extent.x, write.extent.y, write.extent.z};
  size_t row_pitch = static_cast<size_t>(write.row_size);
  size_t slice_pitch = static_cast<size_t>(write.slice_size);
  const uint8_t *pointer = static_cast<const uint8_t *>(write.pointer);

  libimg::HostWriteImage(&image->image, origin, region, row_pitch, slice_pitch,
                         pointer);
#else
  (void)info;
#endif
}

void commandFillImage(host::command_info_s *info) {
#ifdef HOST_IMAGE_SUPPORT
  const host::command_info_fill_image_s &fill = info->fill_image_command;

  auto image = static_cast<host::image_s *>(fill.image);

  size_t origin[3] = {fill.offset.x, fill.offset.y, fill.offset.z};
  size_t region[3] = {fill.extent.x, fill.extent.y, fill.extent.z};
  libimg::HostFillImage(&image->image, fill.color, origin, region);
#else
  (void)info;
#endif
}

void commandCopyImage(host::command_info_s *info) {
#ifdef HOST_IMAGE_SUPPORT
  host::command_info_copy_image_s *const copy = &(info->copy_image_command);

  auto srcImage = static_cast<host::image_s *>(copy->src_image);
  auto dstImage = static_cast<host::image_s *>(copy->dst_image);

  size_t srcOrigin[3] = {copy->src_offset.x, copy->src_offset.y,
                         copy->src_offset.z};
  size_t dstOrigin[3] = {copy->dst_offset.x, copy->dst_offset.y,
                         copy->dst_offset.z};
  size_t region[3] = {copy->extent.x, copy->extent.y, copy->extent.z};
  libimg::HostCopyImage(&srcImage->image, &dstImage->image, srcOrigin,
                        dstOrigin, region);
#else
  (void)info;
#endif
}

void commandCopyImageToBuffer(host::command_info_s *info) {
#ifdef HOST_IMAGE_SUPPORT
  const host::command_info_copy_image_to_buffer_s &copy =
      info->copy_image_to_buffer_command;

  auto srcImage = static_cast<host::image_s *>(copy.src_image);
  auto dstBuffer = static_cast<host::buffer_s *>(copy.dst_buffer);

  size_t srcOrigin[3] = {copy.src_offset.x, copy.src_offset.y,
                         copy.src_offset.z};
  size_t region[3] = {copy.extent.x, copy.extent.y, copy.extent.z};
  size_t dstOffset = static_cast<size_t>(copy.dst_offset);
  libimg::HostCopyImageToBuffer(&srcImage->image, dstBuffer->data, srcOrigin,
                                region, dstOffset);
#else
  (void)info;
#endif
}

void commandCopyBufferToImage(host::command_info_s *info) {
#ifdef HOST_IMAGE_SUPPORT
  host::command_info_copy_buffer_to_image_s *copy =
      &(info->copy_buffer_to_image_command);

  auto srcBuffer = static_cast<host::buffer_s *>(copy->src_buffer);
  auto dstImage = static_cast<host::image_s *>(copy->dst_image);

  size_t dstOrigin[3] = {copy->dst_offset.x, copy->dst_offset.y,
                         copy->dst_offset.z};
  size_t region[3] = {copy->extent.x, copy->extent.y, copy->extent.z};

  libimg::HostCopyBufferToImage(srcBuffer->data, &dstImage->image,
                                copy->src_offset, dstOrigin, region);
#else
  (void)info;
#endif
}

void commandNDRange(host::queue_s *queue, host::command_info_s *info) {
  host::command_info_ndrange_s *const ndrange = &(info->ndrange_command);

  auto host_kernel = static_cast<host::kernel_s *>(ndrange->kernel);

  auto host_device = static_cast<host::device_s *>(queue->device);

  const size_t slices =
      host_device->thread_pool.num_threads() * slice_multiplier;

  host::kernel_variant_s variant;
  if (mux_success != host_kernel->getKernelVariantForWGSize(
                         info->ndrange_command.ndrange_info->local_size[0],
                         info->ndrange_command.ndrange_info->local_size[1],
                         info->ndrange_command.ndrange_info->local_size[2],
                         &variant)) {
    return;
  }

  constexpr size_t signal_count =
      host::thread_pool_s::max_num_threads * slice_multiplier;
  std::array<std::atomic<bool>, signal_count> signals;
  std::atomic<uint32_t> queued(0);
  host_device->thread_pool.enqueue_range(
      [](void *const in, void *const info, void *fence, size_t index) {
        auto *const kernel_variant = static_cast<host::kernel_variant_s *>(in);
        auto *const ndrange = static_cast<host::command_info_ndrange_s *>(info);
        auto *const ndrange_info = ndrange->ndrange_info;
        auto host_device =
            static_cast<host::device_s *>(ndrange->kernel->device);

        for (uint8_t k = 0; k < ndrange_info->dimensions; ++k) {
          if (ndrange_info->global_size[k] == 0) {
            return;
          }
        }

        host::schedule_info_s schedule_info;

        for (uint8_t k = 0; k < 3; k++) {
          schedule_info.global_size[k] = ndrange_info->global_size[k];
          schedule_info.global_offset[k] = ndrange_info->global_offset[k];
          schedule_info.local_size[k] = ndrange_info->local_size[k];
        }
        schedule_info.slice = index;
        schedule_info.total_slices =
            (host_device->thread_pool.num_threads() * slice_multiplier);
        schedule_info.work_dim =
            static_cast<uint32_t>(ndrange_info->dimensions);

        kernel_variant->hook(ndrange_info->packed_args, &schedule_info);
      },
      &variant, ndrange, signals, &queued, slices);

  // Ensure all threads to be done with 'queued' by the time it gets destroyed.
  host_device->thread_pool.wait(&queued);
  {
    std::unique_lock<std::mutex> lock(host_device->thread_pool.wait_mutex);
    host_device->thread_pool.finished.wait(lock,
                                           [&queued] { return queued == 0; });
  }

  // We do need to wait on for 'queued' to be 0 explicitly here, despite the
  // thread_pool.wait and the fact that each signals[i] was set under the same
  // lock that is used when changing 'queued'. This was discovered as seeing
  // that another thread managed to somehow trigger the counter ('queued')
  // after it being freed (exiting the scope of the `commandNDRange` function).
  //
  // It was previously assumed that when the final signal that was signalled
  // 'queued' was set to 0 under the same lock and thus 'queued' must always be
  // zero by the time the above thread_pool.wait() returned, therefore further
  // investigation is required on this to get rid of the inneficiency of the
  // extra atomic synchronisation used to guarantee the thread-safety here.
  assert(0 == queued);
}

void commandUserCallback(host::queue_s *queue, host::command_info_s *info,
                         host::command_buffer_s *command_buffer) {
  host::command_info_user_callback_s *const user_callback =
      &(info->user_callback_command);

  (user_callback->user_function)(queue, command_buffer,
                                 user_callback->user_data);
}

[[nodiscard]] mux_query_duration_result_t commandBeginQuery(
    host::command_info_s *info, mux_query_duration_result_t duration_query) {
  host::command_info_begin_query_s *const begin_query =
      &(info->begin_query_command);
  if (begin_query->pool->type == mux_query_type_duration) {
    return static_cast<host::query_pool_s *>(begin_query->pool)
        ->getDurationQueryAt(begin_query->index);
  }
  return duration_query;
}

[[nodiscard]] mux_query_duration_result_t commandEndQuery(
    host::command_info_s *info, mux_query_duration_result_t duration_query) {
  host::command_info_end_query_s *const end_query = &(info->end_query_command);
  if (end_query->pool->type == mux_query_type_duration) {
    auto end_duration_query = static_cast<host::query_pool_s *>(end_query->pool)
                                  ->getDurationQueryAt(end_query->index);
    if (duration_query == end_duration_query) {
      return nullptr;
    }
  }
  return duration_query;
}

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
void commandBeginQuery(host::command_info_s *info) {
  host::command_info_begin_query_s *const begin_query =
      &(info->begin_query_command);
  auto query_pool = static_cast<host::query_pool_s *>(begin_query->pool);
  query_pool->startEvents();
}

void commandEndQuery(host::command_info_s *info) {
  host::command_info_end_query_s *const end_query = &(info->end_query_command);
  auto query_pool = static_cast<host::query_pool_s *>(end_query->pool);
  query_pool->endEvents();
}
#endif

void commandResetQueryPool(host::command_info_s *info) {
  host::command_info_reset_query_pool_s *const reset_query_pool =
      &(info->reset_query_pool_command);
  auto query_pool = static_cast<host::query_pool_s *>(reset_query_pool->pool);
  query_pool->reset(reset_query_pool->index, reset_query_pool->count);
}

void threadPoolProcessCommands(void *const v_queue,
                               void *const v_command_buffer,
                               void *const v_fence, size_t) {
  auto queue = static_cast<host::queue_s *>(v_queue);
  auto command_buffer = static_cast<host::command_buffer_s *>(v_command_buffer);

  mux_query_duration_result_t duration_query = nullptr;

  for (uint64_t i = 0, e = command_buffer->commands.size(); i < e; i++) {
    host::command_info_s *const info = &(command_buffer->commands[i]);

    uint64_t start = 0;
    if (duration_query) {
      start = utils::timestampNanoSeconds();
    }

    switch (info->type) {
      default:
        return;
      case host::command_type_read_buffer:
        commandReadBuffer(info);
        break;
      case host::command_type_write_buffer:
        commandWriteBuffer(info);
        break;
      case host::command_type_fill_buffer:
        commandFillBuffer(info);
        break;
      case host::command_type_copy_buffer:
        commandCopyBuffer(info);
        break;
      case host::command_type_read_image:
        commandReadImage(info);
        break;
      case host::command_type_write_image:
        commandWriteImage(info);
        break;
      case host::command_type_fill_image:
        commandFillImage(info);
        break;
      case host::command_type_copy_image:
        commandCopyImage(info);
        break;
      case host::command_type_copy_image_to_buffer:
        commandCopyImageToBuffer(info);
        break;
      case host::command_type_copy_buffer_to_image:
        commandCopyBufferToImage(info);
        break;
      case host::command_type_ndrange:
        commandNDRange(queue, info);
        break;
      case host::command_type_user_callback:
        commandUserCallback(queue, info, command_buffer);
        break;
      case host::command_type_begin_query:
        if (info->end_query_command.pool->type == mux_query_type_duration) {
          duration_query = commandBeginQuery(info, duration_query);
        }
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
        if (info->end_query_command.pool->type == mux_query_type_counter) {
          commandBeginQuery(info);
        }
#endif
        break;
      case host::command_type_end_query:
        if (info->end_query_command.pool->type == mux_query_type_duration) {
          duration_query = commandEndQuery(info, duration_query);
        }
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
        if (info->end_query_command.pool->type == mux_query_type_counter) {
          commandEndQuery(info);
        }
#endif
        break;
      case host::command_type_reset_query_pool:
        commandResetQueryPool(info);
        break;
    }

    if (duration_query) {
      auto end = utils::timestampNanoSeconds();
      duration_query->start = start;
      duration_query->end = end;
    }
  }

  threadPoolCleanup(v_queue, v_command_buffer, v_fence, false);
}
}  // namespace

namespace host {
queue_s::queue_s(mux_allocator_info_t allocator, mux_device_t device)
    : runningGroups(0), signalInfos(allocator) {
  this->device = device;
}

queue_s::~queue_s() {}

void queue_s::signalCompleted(mux_command_buffer_t group, bool terminate) {
  auto signalInfo = std::find_if(signalInfos.begin(), signalInfos.end(),
                                 [group](decltype(*signalInfos.begin()) &info) {
                                   return group == info.first;
                                 });
  if (signalInfos.end() != signalInfo) {
    auto hostDevice = static_cast<device_s *>(group->device);
    auto hostGroup = static_cast<command_buffer_s *>(group);
    auto *hostFence = static_cast<fence_s *>(signalInfo->second.fence);
    auto *threadPoolSignal =
        hostFence ? &hostFence->thread_pool_signal : nullptr;

    if (terminate) {
      // and fire off a no-op enqueue to the thread pool because another thread
      // could already be waiting for the group via the thread pool, so we need
      // to signal wait complete in the normal way.
      hostDevice->thread_pool.enqueue(threadPoolCleanup, this, hostGroup,
                                      hostFence, true, threadPoolSignal,
                                      &this->runningGroups);
    } else {
      // we got a signal, so decrement the wait count
      (signalInfo->second.wait_count)--;

      // if we were the last signal on the group, run it!
      if (0 == signalInfo->second.wait_count) {
        hostDevice->thread_pool.enqueue(threadPoolProcessCommands, this,
                                        hostGroup, hostFence, false,
                                        threadPoolSignal, &this->runningGroups);

        // lastly wipe the tracking info for the group
        signalInfos.erase(signalInfo);
        return;
      }
    }
  }
}

mux_result_t queue_s::addGroup(mux_command_buffer_t group, mux_fence_t fence,
                               uint64_t numWaits) {
  if (0 == numWaits) {
    auto *hostDevice = static_cast<device_s *>(group->device);
    auto *hostGroup = static_cast<command_buffer_s *>(group);
    auto *hostFence = static_cast<fence_s *>(fence);
    auto *hostThreadPoolSignal =
        hostFence ? &hostFence->thread_pool_signal : nullptr;
    hostDevice->thread_pool.enqueue(threadPoolProcessCommands, this, hostGroup,
                                    hostFence, 0, hostThreadPoolSignal,
                                    &this->runningGroups);
  } else {
    const signal_info_s signal_info{numWaits, fence};
    if (signalInfos.emplace_back(group, signal_info)) {
      return mux_error_out_of_memory;
    }
  }

  return mux_success;
}
}  // namespace host

mux_result_t hostGetQueue(mux_device_t device, mux_queue_type_e, uint32_t,
                          mux_queue_t *out_queue) {
  auto hostDevice = static_cast<host::device_s *>(device);

  *out_queue = &(hostDevice->queue);

  return mux_success;
}

mux_result_t hostDispatch(
    mux_queue_t queue, mux_command_buffer_t command_buffer, mux_fence_t fence,
    mux_semaphore_t *wait_semaphores, uint32_t wait_semaphores_length,
    mux_semaphore_t *signal_semaphores, uint32_t signal_semaphores_length,
    void (*user_function)(mux_command_buffer_t command_buffer,
                          mux_result_t error, void *const user_data),
    void *user_data) {
  auto hostGroup = static_cast<host::command_buffer_s *>(command_buffer);
  auto hostQueue = static_cast<host::queue_s *>(queue);
  auto hostFence = static_cast<host::fence_s *>(fence);

  const std::lock_guard<std::mutex> guard(hostQueue->mutex);

  // store the semaphores we have to signal into the group
  if (!hostGroup->signal_semaphores.insert(
          hostGroup->signal_semaphores.end(), signal_semaphores,
          signal_semaphores + signal_semaphores_length)) {
    return mux_error_out_of_memory;
  }

  hostGroup->user_function = user_function;
  hostGroup->user_data = user_data;

  // The fence is optional, it may be null.
  if (hostFence) {
    hostFence->reset();
  }

  // track the group in the queue...
  hostQueue->addGroup(command_buffer, hostFence, wait_semaphores_length);

  // ...then tell the semaphores in the wait list about the group
  for (uint64_t i = 0; i < wait_semaphores_length; i++) {
    auto *semaphore = static_cast<host::semaphore_s *>(wait_semaphores[i]);
    semaphore->addWait(command_buffer);
  }

  return mux_success;
}

mux_result_t hostTryWait(mux_queue_t queue, uint64_t timeout,
                         mux_fence_t fence) {
  (void)queue;

  auto *hostFence = static_cast<host::fence_s *>(fence);
  return hostFence->tryWait(timeout);
}

mux_result_t hostWaitAll(mux_queue_t queue) {
  auto host = static_cast<host::queue_s *>(queue);
  auto hostDevice = static_cast<host::device_s *>(host->device);
  auto &hostPool = hostDevice->thread_pool;

  // Wait for all work to have left the thread pool, this occurs when the
  // runningGroups atomic reaches zero.
  std::unique_lock<std::mutex> lock(hostPool.wait_mutex);
  hostPool.finished.wait(lock, [host] { return 0 == host->runningGroups; });

  return mux_success;
}
