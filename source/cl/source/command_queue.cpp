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

#include <cl/buffer.h>
#include <cl/command_queue.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/image.h>
#include <cl/kernel.h>
#include <cl/mux.h>
#include <cl/platform.h>
#include <cl/program.h>
#include <cl/semaphore.h>
#include <cl/validate.h>
#include <extension/extension.h>
#include <tracer/tracer.h>
#include <utils/system.h>

#include "mux/mux.h"

_cl_command_queue::_cl_command_queue(cl_context context, cl_device_id device,
                                     cl_command_queue_properties properties,
                                     mux_queue_t mux_queue)
    : base(cl::ref_count_type::EXTERNAL),
      context(context),
      device(device),
      properties(properties),
      profiling_start(
          cl::validate::IsInBitSet(properties, CL_QUEUE_PROFILING_ENABLE)
              ? utils::timestampNanoSeconds()
              : 0),
      mux_queue(mux_queue),
      counter_queries(nullptr),
      pending_command_buffers(),
      pending_dispatches(),
      running_command_buffers(),
      finish_state(),
      cached_command_buffers(),
      in_flush(false) {
  cl::retainInternal(context);
  cl::retainInternal(device);
}

_cl_command_queue::~_cl_command_queue() {
  muxWaitAll(mux_queue);

  {
    const std::lock_guard<std::mutex> lock(context->getCommandQueueMutex());
    cleanupCompletedCommandBuffers();
  }
  // Release any completed signal semaphores
  for (auto semaphore : completed_signal_semaphores) {
    releaseSemaphore(semaphore);
  }

  for (auto pair : fences) {
    muxDestroyFence(device->mux_device, pair.second, device->mux_allocator);
  }

  // Empty our command buffer cache.
  while (true) {
    // Can access unlocked because if the destructor is running in parallel to
    // other method on this object something has gone really wrong anyway.
    auto command_buffer = cached_command_buffers.dequeue();
    if (!command_buffer) {
      break;
    }
    muxDestroyCommandBuffer(device->mux_device, *command_buffer,
                            device->mux_allocator);
  }

  if (counter_queries) {
    muxDestroyQueryPool(mux_queue, counter_queries, device->mux_allocator);
  }

  cl::releaseInternal(device);
  cl::releaseInternal(context);
}

// Used by clCreateCommandQueue.
cargo::expected<std::unique_ptr<_cl_command_queue>, cl_int>
_cl_command_queue::create(cl_context context, cl_device_id device,
                          cl_command_queue_properties properties) {
  if (properties &
      ~(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE)) {
    return cargo::make_unexpected(CL_INVALID_VALUE);
  }

#ifndef CA_ENABLE_OUT_OF_ORDER_EXEC_MODE
  if (cl::validate::IsInBitSet(properties,
                               CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)) {
    return cargo::make_unexpected(CL_INVALID_QUEUE_PROPERTIES);
  }
#endif

  mux_queue_t mux_queue;
  const mux_result_t error =
      muxGetQueue(device->mux_device, mux_queue_type_compute, 0, &mux_queue);
  OCL_CHECK(error, return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY));

  auto queue = std::unique_ptr<_cl_command_queue>(new (
      std::nothrow) _cl_command_queue(context, device, properties, mux_queue));
  OCL_CHECK(nullptr == queue,
            return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY));

  return queue;
}

// Used by clCreateCommandQueueWithProperties and
// clCreateCommandQueueWithPropertiesKHR.
cargo::expected<std::unique_ptr<_cl_command_queue>, cl_int>
_cl_command_queue::create(cl_context context, cl_device_id device,
                          const cl_bitfield *properties) {
  mux_queue_t mux_queue;
  const mux_result_t error =
      muxGetQueue(device->mux_device, mux_queue_type_compute, 0, &mux_queue);
  OCL_CHECK(error, return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY));

  auto command_queue = std::unique_ptr<_cl_command_queue>(
      new (std::nothrow) _cl_command_queue(context, device, 0, mux_queue));
  OCL_CHECK(!command_queue,
            return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY));

  if (properties && properties[0] != 0) {
    // Mask of all the valid bits in cl_command_queue_properties used to catch
    // any invalid bits we might get.
    cl_command_queue_properties command_queue_properties = 0;
    const cl_command_queue_properties valid_properties_mask =
#if defined(CL_VERSION_3_0)
        CL_QUEUE_ON_DEVICE | CL_QUEUE_ON_DEVICE_DEFAULT |
#endif
        CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE;
    auto current = properties;
    do {
      const cl_bitfield property = current[0];
      const cl_command_queue_properties value = current[1];
      switch (property) {
        case CL_QUEUE_PROPERTIES:
          if (value & ~valid_properties_mask) {
            return cargo::make_unexpected(CL_INVALID_VALUE);
#ifndef CA_ENABLE_OUT_OF_ORDER_EXEC_MODE
          } else if (value & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) {
            // TODO(CA-1123): Support out of order command queues.
            return cargo::make_unexpected(CL_INVALID_QUEUE_PROPERTIES);
#endif
#if defined(CL_VERSION_3_0)
          } else if (value & CL_QUEUE_ON_DEVICE ||
                     value & CL_QUEUE_ON_DEVICE_DEFAULT) {
            return cargo::make_unexpected(CL_INVALID_QUEUE_PROPERTIES);
#endif
          } else {
            command_queue_properties |= value;
          }
          break;
#if defined(CL_VERSION_3_0)
        case CL_QUEUE_SIZE:
          return cargo::make_unexpected(CL_INVALID_QUEUE_PROPERTIES);
#endif
        default:
          // Extensions can add support for additional properties, do not
          // remove the call to this function.
          if (auto error = extension::ApplyPropertyToCommandQueue(
                  command_queue.get(), property, value)) {
            return cargo::make_unexpected(error);
          }
          break;
      }
      current += 2;
    } while (current[0] != 0);
    command_queue->properties = command_queue_properties;
#if defined(CL_VERSION_3_0)
    if (command_queue->properties_list.assign(properties, current + 1)) {
      return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
    }
#endif
  }

  return command_queue;
}

cl_int _cl_command_queue::flush() {
  if (in_flush) {
    return CL_SUCCESS;
  }
  in_flush = true;

  // Use a raii_wrapper to ensure in_flush is set to false on exit
  struct raii_wrapper {
    raii_wrapper(bool &in_flush) : in_flush(in_flush) {};
    ~raii_wrapper() { in_flush = false; }
    bool &in_flush;
  };
  const raii_wrapper wrapper(in_flush);

  if (auto error = cleanupCompletedCommandBuffers()) {
    return error;
  }

  {
    if (!pending_dispatches.empty()) {
      cargo::small_vector<mux_command_buffer_t, 16> command_buffers;
      if (command_buffers.reserve(pending_command_buffers.size())) {
        return CL_OUT_OF_RESOURCES;
      }

      // Filter out all pending_dispatches which depend on user events.
      for (auto &command_buffer : pending_command_buffers) {
        auto &dispatch = pending_dispatches[command_buffer];
        if (std::none_of(dispatch.wait_events.begin(),
                         dispatch.wait_events.end(), cl::isUserEvent)) {
          for (auto &wait_event : dispatch.wait_events) {
            // Force a flush if from a different queue
            if (CL_COMMAND_USER != wait_event->command_type &&
                wait_event->command_status != CL_COMPLETE) {
              if (wait_event->queue != this) {
                wait_event->queue->flush();
              }
            }
          }
          if (command_buffers.push_back(command_buffer)) {
            return CL_OUT_OF_RESOURCES;
          }
        }
      }

      // Dispatch the command buffers which don't depend on user events.
      if (auto error = dispatch(command_buffers)) {
        return error;
      }
    }
  }

  return CL_SUCCESS;
}

cl_int _cl_command_queue::waitForEvents(const cl_uint num_events,
                                        const cl_event *events) {
  for (cl_uint i = 0; i < num_events; i++) {
    events[i]->wait();
  }
  const std::lock_guard<std::mutex> lock(context->getCommandQueueMutex());

  return CL_SUCCESS == cleanupCompletedCommandBuffers()
             ? CL_SUCCESS
             : CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
}

cl_int _cl_command_queue::getEventStatus(cl_event event) {
  const std::lock_guard<std::mutex> lock(context->getCommandQueueMutex());
  const cl_int error = cleanupCompletedCommandBuffers();
  OCL_UNUSED(error);
  assert(CL_SUCCESS == error);
  return event->command_status;
}

cl_int _cl_command_queue::cleanupCompletedCommandBuffers() {
  // Check to see if there are any command buffers ready to be cleaned up.
  while (true) {
    if (running_command_buffers.empty()) {
      // There are no running command buffers so we can stop processing.
      break;
    }

    // Check if the first running command buffer has completed.
    auto fence = fences[running_command_buffers.front().command_buffer];
    assert(fence && "Missing fence entry for command buffer dispatch!");
    const mux_result_t error = muxTryWait(mux_queue, 0, fence);
    OCL_ASSERT(mux_success == error || mux_error_fence_failure == error ||
                   mux_fence_not_ready == error,
               "muxTryWait failed!");

    if (mux_fence_not_ready == error) {
      // The command buffer wasn't yet complete. Because of how our command
      // groups are linearly chained together (we have an in order queue)
      // we can bail now as if this command buffer isn't complete, future
      // ones will not have completed yet either.
      return CL_SUCCESS;
    }

    // The command buffer has either failed or completed, so delete the fence
    // and remove the associated entry from the map.
    // TODO: We could do better here and reset the fences then reuse them.
    muxDestroyFence(device->mux_device, fence, device->mux_allocator);
    fences.erase(running_command_buffers.front().command_buffer);

    // Note that by this point 'error' may be either mux_success or
    // mux_error_fence_failure.  This function does not care about
    // the difference (the error is handled elsewhere), we just consider
    // either case to mean that the group is 'complete' and process it
    // accordingly.

    // The command buffer has completed so stop tracking it then destroy it.
    auto completed = std::move(running_command_buffers.front());
    // Any completed buffers that have wait semaphores should be cleaned
    // up
    for (auto &s : completed.wait_semaphores) {
      releaseSemaphore(s);
    }
    running_command_buffers.pop_front();

#ifdef OCL_EXTENSION_cl_khr_command_buffer
    // We need to release references on any command buffers associated with user
    // command buffers even if they are cloned.
    if (completed.is_user_command_buffer) {
      user_command_buffers[completed.command_buffer]->execution_refcount--;
      cl::releaseInternal(user_command_buffers[completed.command_buffer]);
      user_command_buffers.erase(completed.command_buffer);
    }
#endif

    // We shouldn't destroy non cloned mux_command_buffers associated with
    // cl_command_buffer_khrs here, they are responsible for their own
    // destruction.
    if (completed.should_destroy_command_buffer) {
      if (auto error = destroyCommandBuffer(completed.command_buffer)) {
        return error;
      }
    }

    // Remove the signal semaphore from pending dispatches.
    for (auto &dispatch : pending_dispatches) {
      auto &wait_semaphores = dispatch.second.wait_semaphores;
      auto found = std::find(wait_semaphores.begin(), wait_semaphores.end(),
                             completed.signal_semaphore);
      if (found != wait_semaphores.end()) {
        releaseSemaphore(*found);
        wait_semaphores.erase(found);
      }
    }

    // Append the completed signal semaphore to the cleanup list.
    if (completed_signal_semaphores.push_back(completed.signal_semaphore)) {
      return CL_OUT_OF_HOST_MEMORY;
    }

    // Get list of all wait_semaphores from running dispatches.
    cargo::small_vector<mux_shared_semaphore, 64> running_wait_semaphores;
    for (auto &running : running_command_buffers) {
      if (!running_wait_semaphores.insert(running_wait_semaphores.end(),
                                          running.wait_semaphores.begin(),
                                          running.wait_semaphores.end())) {
        return CL_OUT_OF_HOST_MEMORY;
      }
    }
    // Remove any duplicates from the list of all running wait semaphores.
    running_wait_semaphores.erase(std::unique(running_wait_semaphores.begin(),
                                              running_wait_semaphores.end()),
                                  running_wait_semaphores.end());

    // Iterate over completed signal semaphores and release them.

    // Store destroyed semaphores for later processing.
    cargo::small_vector<mux_shared_semaphore, 16> released_semaphores;
    for (auto signal_semaphore : completed_signal_semaphores) {
      // Release the semaphore now that nothing depends on it.
      if (auto error = releaseSemaphore(signal_semaphore)) {
        return error;
      }
      // Append to the list of destroyed semaphores to be removed from
      // completed signal semaphores list.
      if (released_semaphores.push_back(signal_semaphore)) {
        return CL_OUT_OF_HOST_MEMORY;
      }
    }

    // Move destroyed semaphores to the back, then erase them.
    auto first_released_semaphore = std::stable_partition(
        completed_signal_semaphores.begin(), completed_signal_semaphores.end(),
        [&released_semaphores](mux_shared_semaphore semaphore) {
          // When semaphore is not in released_semaphores return true which
          // moves it to the front, otherwise move it to the back.
          return std::find(released_semaphores.begin(),
                           released_semaphores.end(),
                           semaphore) == released_semaphores.end();
        });
    completed_signal_semaphores.erase(first_released_semaphore,
                                      completed_signal_semaphores.end());
  }

  return CL_SUCCESS;
}

cl_uint _cl_command_queue::getDeviceIndex() {
  return context->getDeviceIndex(device);
}

[[nodiscard]] cargo::expected<mux_command_buffer_t, cl_int>
_cl_command_queue::getCommandBuffer(
    cargo::array_view<const cl_event> event_wait_list, cl_event event) {
  // Register the wait and signal events for the command buffer's dispatch.
  auto registerEvents = [this, event_wait_list,
                         event](mux_command_buffer_t command_buffer)
      -> cargo::expected<mux_command_buffer_t, cl_int> {
    auto &dispatch = pending_dispatches[command_buffer];
    if (auto error = dispatch.addWaitEvents(event_wait_list)) {
      return cargo::make_unexpected(error);
    }
    if (auto error = dispatch.addSignalEvent(event)) {
      return cargo::make_unexpected(error);
    }
    if (event && (properties & CL_QUEUE_PROFILING_ENABLE)) {
      if (auto mux_error = muxCommandBeginQuery(
              command_buffer, event->profiling.duration_queries, 0, 1, 0,
              nullptr, nullptr)) {
        return cargo::make_unexpected(cl::getErrorFrom(mux_error));
      }
    }
    return command_buffer;
  };

  auto setEventFailure = [event](cl_int error) {
    if (event) {
      event->complete(error);
    }
  };

  return getCommandBufferPending(event_wait_list)
      .and_then(registerEvents)
      .or_else(setEventFailure);
}

[[nodiscard]] cl_int _cl_command_queue::registerDispatchCallback(
    mux_command_buffer_t command_buffer, cl_event event,
    std::function<void()> callback) {
  OCL_ASSERT(
      pending_dispatches.end() != pending_dispatches.find(command_buffer),
      "command_buffer not found in pending_dispatches");
  if (event && (properties & CL_QUEUE_PROFILING_ENABLE)) {
    if (auto mux_error = muxCommandEndQuery(command_buffer,
                                            event->profiling.duration_queries,
                                            0, 1, 0, nullptr, nullptr)) {
      return cl::getErrorFrom(mux_error);
    }
  }
  if (pending_dispatches[command_buffer].addCallback(std::move(callback))) {
    return CL_OUT_OF_RESOURCES;
  }
  return CL_SUCCESS;
}

[[nodiscard]] cargo::expected<mux_command_buffer_t, cl_int>
_cl_command_queue::getCurrentCommandBuffer() {
  if (pending_command_buffers.empty()) {
    // There are no pending command buffers, create one.
    return createCommandBuffer();
  }
  // Since we only support in-order command queues the most recently created
  // command buffer is the current one.
  return pending_command_buffers.back();
}

[[nodiscard]] cargo::expected<mux_command_buffer_t, cl_int>
_cl_command_queue::getCommandBufferPending(
    cargo::array_view<const cl_event> event_wait_list) {
  // Utility function object adds wait semaphores to a pending dispatch.
  struct add_wait {
    add_wait(cargo::array_view<mux_shared_semaphore> semaphores,
             std::unordered_map<mux_command_buffer_t, dispatch_state_t>
                 &pending_dispatches)
        : semaphores(semaphores), pending_dispatches(pending_dispatches) {}

    cargo::expected<mux_command_buffer_t, cl_int> operator()(
        mux_command_buffer_t command_buffer) {
      if (!semaphores.empty()) {
        auto &dispatch = pending_dispatches[command_buffer];
        auto wait_sems_size = dispatch.wait_semaphores.size();

        // Insert the wait semaphores into the list.
        if (!dispatch.wait_semaphores.insert(dispatch.wait_semaphores.end(),
                                             semaphores.begin(),
                                             semaphores.end())) {
          return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
        }
        // Ensure there are no duplicate wait semaphores in the list.
        auto end = std::unique(dispatch.wait_semaphores.begin(),
                               dispatch.wait_semaphores.end());
        if (auto count = std::distance(end, dispatch.wait_semaphores.end())) {
          if (dispatch.wait_semaphores.resize(dispatch.wait_semaphores.size() -
                                              count)) {
            return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
          }
        }

        // After duplicate remove, retain any semaphores we have genuinely added
        auto current_sems_size = dispatch.wait_semaphores.size();
        for (unsigned int index = wait_sems_size; index < current_sems_size;
             index++) {
          dispatch.wait_semaphores[index]->retain();
        }
      }
      return command_buffer;
    }

    cargo::array_view<mux_shared_semaphore> semaphores;
    std::unordered_map<mux_command_buffer_t, dispatch_state_t>
        &pending_dispatches;
  };

  // Storage for the pending dispatches on which this command buffer will
  // depend.
  using dispatch_pair = std::pair<const mux_command_buffer_t, dispatch_state_t>;
  using dispatch_dependency = std::pair<dispatch_pair *, cl_command_queue>;
  cargo::small_vector<dispatch_dependency, 8> dependent_dispatches;

  // Flag indicating whether it is safe to append to the last command buffer in
  // the case that we only have one dependent command (which will always be the
  // last dispatch).
  bool can_append_last_dispatch = true;

  // We always need to wait on the last pending dispatch (if there is one).
  if (!pending_command_buffers.empty()) {
    auto &pending_command_buffer = pending_command_buffers.back();
    auto pending_dispatch = pending_dispatches.find(pending_command_buffer);
    OCL_ASSERT(pending_dispatch != std::end(pending_dispatches),
               "The last pending command buffer has no entry in the "
               "pending dispatches map.");
    if (dependent_dispatches.push_back({&*pending_dispatch, this})) {
      return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
    }

    // We can't append to the last pending dispatch if it is a user command
    // buffer.
    can_append_last_dispatch = !pending_dispatch->second.is_user_command_buffer;
  }

  cargo::small_vector<cl_command_queue, 2> dependent_dispatch_command_queues;
  // Find all dependent dispatches in the event_wait_list.
  for (auto wait_event : event_wait_list) {
    if (cl::isUserEvent(wait_event) &&
        wait_event->command_status != CL_COMPLETE) {
      // We can't append to the last dispatch if we need to wait on a user
      // event.
      can_append_last_dispatch = false;
      if (!wait_event->addCallback(CL_COMPLETE, &userEventDispatch, this)) {
        return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
      }
    }

    auto isWaitEvent = [wait_event](const cl_event signal_event) {
      return wait_event == signal_event;
    };

    for (auto &pending : pending_dispatches) {
      auto &dispatch = pending.second;
      // Check if any signal events are the wait event and add them to
      // dependent_dispatches.
      if (std::any_of(dispatch.signal_events.begin(),
                      dispatch.signal_events.end(), isWaitEvent)) {
        if (dependent_dispatches.push_back({&pending, wait_event->queue})) {
          return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
        }
      }
    }

    // Check the pending dispatches of the wait event's queue if it is
    // different from this one
    if (wait_event->queue != this &&
        CL_COMMAND_USER != wait_event->command_type) {
      // Check if the cross queue does not exist in the queues
      if (std::find(dependent_dispatch_command_queues.begin(),
                    dependent_dispatch_command_queues.end(),
                    wait_event->queue) ==
          dependent_dispatch_command_queues.end()) {
        if (dependent_dispatch_command_queues.push_back(wait_event->queue)) {
          return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
        }
      }
      can_append_last_dispatch = false;

      for (auto &pending : wait_event->queue->pending_dispatches) {
        auto &dispatch = pending.second;
        // Check if any signal events are the wait event and add them to
        // dependent_dispatches.
        if (std::any_of(dispatch.signal_events.begin(),
                        dispatch.signal_events.end(), isWaitEvent)) {
          if (dependent_dispatches.push_back({&pending, wait_event->queue})) {
            return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
          }
        }
      }
    }
  }

  // Remove duplicates from dependent_dispatches.
  auto end =
      std::unique(dependent_dispatches.begin(), dependent_dispatches.end());
  if (auto extra = std::distance(end, dependent_dispatches.end())) {
    if (dependent_dispatches.resize(dependent_dispatches.size() - extra)) {
      return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
    }
  }

  // There is only a single dependent dispatch so return its command buffer.
  // Since there is only one it must be the most recent dispatch.
  if (dependent_dispatches.size() == 1 && can_append_last_dispatch) {
    return dependent_dispatches.front().first->first;
  }

  // Storage for wait semaphores to set on a pending command buffer.
  cargo::small_vector<mux_shared_semaphore, 8> semaphores;

  // We always process the current queue
  if (dependent_dispatch_command_queues.push_back(this)) {
    return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
  }

  for (auto &dispatch_queue : dependent_dispatch_command_queues) {
    // There are one or more dependent dispatches we must create a new command
    // group and wait on the their signal semaphores.
    const bool has_dependent_dispatches =
        std::find_if(
            dependent_dispatches.begin(), dependent_dispatches.end(),
            [dispatch_queue](dispatch_dependency &dispatch_dependency) {
              return dispatch_dependency.second == dispatch_queue;
            }) != std::end(dependent_dispatches);

    if (has_dependent_dispatches) {
      for (auto &dependent_dispatch_info : dependent_dispatches) {
        if (dependent_dispatch_info.second != dispatch_queue) {
          continue;
        }
        auto &dependent_dispatch = dependent_dispatch_info.first->second;

        // Append the signal semaphore to wait_semaphores of the current
        // dispatch.
        if (semaphores.push_back(dependent_dispatch.signal_semaphore)) {
          return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
        }
      }
    } else {
      // There are no dependent dispatches, this means the command buffer is
      // running now or has already completed or there were never any wait
      // events in the first place. Wait on all running dispatches to ensure
      // ordering since the commands in running_command_buffers may be out of
      // order with respect the container (ordering is still enforced via
      // semaphore dependencies though).
      for (auto &running_dispatch : dispatch_queue->running_command_buffers) {
        if (semaphores.push_back(running_dispatch.signal_semaphore)) {
          return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
        }
      }
    }
  }

  return createCommandBuffer().and_then(
      add_wait{semaphores, pending_dispatches});
}

[[nodiscard]] cl_int _cl_command_queue::dispatch(
    cargo::array_view<mux_command_buffer_t> command_buffers) {
  for (auto command_buffer : command_buffers) {
    auto &dispatch = pending_dispatches[command_buffer];

    if (counter_queries) {
      if (auto mux_error =
              muxCommandEndQuery(command_buffer, counter_queries, 0,
                                 counter_queries->count, 0, nullptr, nullptr)) {
        return cl::getErrorFrom(mux_error);
      }
    }

    // Finalize non-user command buffers before dispatch.
    if (auto error = muxFinalizeCommandBuffer(command_buffer)) {
      // Make sure we return a valid error code for the calling OpenCL APIs.
      auto cl_error = cl::getErrorFrom(error);
      switch (cl_error) {
        default:
          return CL_INVALID_COMMAND_QUEUE;
        case CL_OUT_OF_RESOURCES:
        case CL_OUT_OF_HOST_MEMORY:
          return cl_error;
      }
    }

    // Create a fence that the host can wait on for this command buffer.
    mux_fence_t fence = nullptr;
    if (auto error =
            muxCreateFence(device->mux_device, device->mux_allocator, &fence)) {
      // Make sure we return a valid error code for the calling OpenCL APIs.
      auto cl_error = cl::getErrorFrom(error);
      switch (cl_error) {
        default:
          return CL_INVALID_COMMAND_QUEUE;
        case CL_OUT_OF_RESOURCES:
        case CL_OUT_OF_HOST_MEMORY:
          return cl_error;
      }
    }

    // Put the fence in the lookup map so we know what fence to wait on for a
    // given command buffer.
    assert(fences.find(command_buffer) == std::end(fences) &&
           "command buffer already has fence entry!");
    fences[command_buffer] = fence;

    // Set all events as submitted.
    for (auto signal_event : dispatch.signal_events) {
      signal_event->submitted();
    }

    for (auto &w : dispatch.wait_events) {
      cl::releaseInternal(w);
    }
    // All wait_events are no longer required past this point.
    dispatch.wait_events.clear();

    // Move dispatched pending state to destruction storage.
    auto &finished = finish_state[command_buffer];
    finished.addState(this, std::move(dispatch.signal_events),
                      std::move(dispatch.callbacks));

    // Completion callback to cleanup once the dispatch is complete.
    auto dispatchComplete = [](mux_command_buffer_t command_buffer,
                               mux_result_t error, void *const user_data) {
      auto finish_state = static_cast<finish_state_t *>(user_data);
      finish_state->clear(command_buffer, error, /* locked */ false);
    };

    // Prepare the dispatch.
    mux_semaphore_t *signal_semaphores = nullptr;
    uint32_t signal_semaphores_length = 0;
    if (dispatch.signal_semaphore) {
      signal_semaphores = &(dispatch.signal_semaphore->semaphore);
      signal_semaphores_length = 1;
    }

    // Actually dispatch the command buffer.
    cargo::small_vector<mux_semaphore_t, 8> wait_semaphores_storage;
    for (auto s : dispatch.wait_semaphores) {
      if (wait_semaphores_storage.push_back(s->get())) {
        return CL_OUT_OF_RESOURCES;
      }
    }

    if (auto error = muxDispatch(
            mux_queue, command_buffer, fence,
            wait_semaphores_storage.empty() ? nullptr
                                            : wait_semaphores_storage.data(),
            dispatch.wait_semaphores.size(), signal_semaphores,
            signal_semaphores_length, dispatchComplete, &finished)) {
      finished.clear(command_buffer, error, /* locked */ true);
      return CL_OUT_OF_RESOURCES;
    }

    // Add to the running double ended queue.
    running_command_buffers.push_back(
        {command_buffer, std::move(dispatch.wait_semaphores),
         dispatch.signal_semaphore, dispatch.is_user_command_buffer,
         dispatch.should_destroy_command_buffer});
  }

  // Remove dispatched command buffers from pending.
  return removeFromPending(command_buffers);
}

cl_int _cl_command_queue::dispatchPending(cl_event user_event) {
  const std::lock_guard<std::mutex> lock(context->getCommandQueueMutex());

  // Remove the user event from all pending dispatches wait event lists.
  for (auto &pending : pending_dispatches) {
    auto &dispatch = pending.second;
    auto found = std::find(dispatch.wait_events.begin(),
                           dispatch.wait_events.end(), user_event);
    if (dispatch.wait_events.end() != found) {
      cl::releaseInternal(*found);
      dispatch.wait_events.erase(found);
    }
  }

  // Flush the command queue, all previously pending dispatches can start.
  return flush();
}

cl_int _cl_command_queue::removeFromPending(
    cargo::array_view<mux_command_buffer_t> command_buffers) {
  if (command_buffers.empty()) {
    return CL_SUCCESS;  // GCOVR_EXCL_LINE non-deterministically executed
  }

  // Remove the command buffers dispatch info.
  for (auto command_buffer : command_buffers) {
    pending_dispatches.erase(command_buffer);
  }

  // Predicate returns `true` if the command buffer should be kept, `false` if
  // it should be removed.
  auto isRetained = [&command_buffers](mux_command_buffer_t command_buffer) {
    return std::none_of(command_buffers.begin(), command_buffers.end(),
                        [command_buffer](mux_command_buffer_t dropped) {
                          return command_buffer == dropped;
                        });
  };

  // Partition the command buffers whilst maintaining original ordering,
  // retained command buffers are placed at the beginning of the range, removed
  // command buffers at the end, the partition point is returned.
  auto end =
      std::stable_partition(std::begin(pending_command_buffers),
                            std::end(pending_command_buffers), isRetained);

  // Resize the vector using the partition point as the new end, removing
  // the command_buffers from pending_command_buffers.
  if (auto extra = std::distance(end, std::end(pending_command_buffers))) {
    if (pending_command_buffers.resize(pending_command_buffers.size() -
                                       extra)) {
      return CL_OUT_OF_RESOURCES;
    }
  }

  return CL_SUCCESS;
}

cl_int _cl_command_queue::dropDispatchesPending(
    cl_event user_event, cl_int event_command_exec_status) {
  const std::lock_guard<std::mutex> lock(context->getCommandQueueMutex());

  cargo::small_vector<mux_command_buffer_t, 16> command_buffers;

  auto isEvent = [user_event](cl_event event) { return user_event == event; };

  for (auto &pending : pending_dispatches) {
    auto command_buffer = pending.first;
    auto &dispatch = pending.second;

    if (std::any_of(dispatch.wait_events.begin(), dispatch.wait_events.end(),
                    isEvent)) {
      // Invoke all completion callbacks in reverse order.
      std::for_each(dispatch.callbacks.rbegin(), dispatch.callbacks.rend(),
                    [](std::function<void()> &callback) { callback(); });
      dispatch.callbacks.clear();

      // Release the signal semaphore if it exists.
      if (auto error = releaseSemaphore(dispatch.signal_semaphore)) {
        return error;
      }
      dispatch.signal_semaphore = nullptr;

      // Mark all signal_events as failed and release them.
      for (auto signal_event : dispatch.signal_events) {
        signal_event->complete(event_command_exec_status);
        cl::releaseInternal(signal_event);
      }
      dispatch.signal_events.clear();
      for (auto &w : dispatch.wait_events) {
        cl::releaseInternal(w);
      }
      dispatch.wait_events.clear();

      for (auto &s : dispatch.wait_semaphores) {
        releaseSemaphore(s);
      }
      dispatch.wait_semaphores.clear();
      // Add command buffer to removal list.
      if (command_buffers.push_back(command_buffer)) {
        return CL_OUT_OF_RESOURCES;
      }

      // All uses of command_buffer after this point are only as an address.
      if (auto error = destroyCommandBuffer(command_buffer)) {
        return error;
      }
    }
  }

  // Remove dropped command buffers from pending.
  return removeFromPending(command_buffers);
}

[[nodiscard]] cargo::expected<mux_command_buffer_t, cl_int>
_cl_command_queue::createCommandBuffer() {
  mux_command_buffer_t command_buffer;
  if (auto cached_command_buffer = cached_command_buffers.dequeue()) {
    // We have a cached command buffer we can use.
    command_buffer = *cached_command_buffer;
  } else {
    // Otherwise create a new command buffer.
    if (mux_success !=
        muxCreateCommandBuffer(device->mux_device, context->getMuxCallback(),
                               device->mux_allocator, &command_buffer)) {
      return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
    }
  }

  // Add the command buffer to the list of pending command buffers.
  if (pending_command_buffers.push_back(command_buffer)) {
    destroyCommandBuffer(command_buffer);
    return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
  }

  // Create a semaphore which future command buffers can wait for.
  auto semaphore = createSemaphore();
  if (!semaphore) {
    destroyCommandBuffer(command_buffer);
    return cargo::make_unexpected(semaphore.error());
  }
  pending_dispatches[command_buffer].signal_semaphore = *semaphore;
  pending_dispatches[command_buffer].is_user_command_buffer = false;
  pending_dispatches[command_buffer].should_destroy_command_buffer = true;

  if (counter_queries) {
    if (auto mux_error =
            muxCommandBeginQuery(command_buffer, counter_queries, 0,
                                 counter_queries->count, 0, nullptr, nullptr)) {
      return cargo::make_unexpected(cl::getErrorFrom(mux_error));
    }
  }

  return command_buffer;
}

cl_int _cl_command_queue::destroyCommandBuffer(
    mux_command_buffer_t command_buffer) {
  // First, reset the command buffer.
  if (mux_success != muxResetCommandBuffer(command_buffer)) {
    // Command buffer reset failed, destroy it.
    muxDestroyCommandBuffer(device->mux_device, command_buffer,
                            device->mux_allocator);

    return CL_OUT_OF_RESOURCES;
  }

  // Try and cache the command buffer first
  if (cargo::result::success !=
      cached_command_buffers.enqueue(command_buffer)) {
    // Then if we have no room to cache it, destroy it.
    muxDestroyCommandBuffer(device->mux_device, command_buffer,
                            device->mux_allocator);
  }

  return CL_SUCCESS;
}

cargo::expected<mux_shared_semaphore, cl_int>
_cl_command_queue::createSemaphore() {
  mux_semaphore_t mux_semaphore;
  if (mux_success != muxCreateSemaphore(device->mux_device,
                                        device->mux_allocator,
                                        &mux_semaphore)) {
    return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
  }
  auto sem = _mux_shared_semaphore::create(device, mux_semaphore);
  if (!sem) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }

  return sem;
}

cl_int _cl_command_queue::releaseSemaphore(mux_shared_semaphore semaphore) {
  const bool should_destroy = semaphore->release();
  if (should_destroy) {
    delete semaphore;
  }
  return CL_SUCCESS;
}

void _cl_command_queue::userEventDispatch(cl_event user_event,
                                          cl_int event_command_exec_status,
                                          void *user_data) {
  auto command_queue = static_cast<cl_command_queue>(user_data);
  if (event_command_exec_status < 0) {
    // User event has a failure status, we can't dispatch.
    command_queue->dropDispatchesPending(user_event, event_command_exec_status);
  }
  // User event succeeded, we can dispatch.
  command_queue->dispatchPending(user_event);
}

[[nodiscard]] cl_int _cl_command_queue::dispatch_state_t::addWaitEvents(
    cargo::array_view<const cl_event> event_wait_list) {
  if (!event_wait_list.empty()) {
    // Add all non-completed events to the dispatches wait list.
    if (wait_events.reserve(wait_events.size() + event_wait_list.size())) {
      return CL_OUT_OF_RESOURCES;
    }
    for (auto wait_event : event_wait_list) {
      // Push any events that are not completed on wait list
      // including non-user ones
      if (wait_event->command_status != CL_COMPLETE) {
        cl::retainInternal(wait_event);
        if (wait_events.push_back(wait_event)) {
          return CL_OUT_OF_RESOURCES;
        }
      }
    }
  }

  return CL_SUCCESS;
}

[[nodiscard]] cl_int _cl_command_queue::dispatch_state_t::addSignalEvent(
    cl_event event) {
  if (event) {
    if (signal_events.push_back(event)) {
      return CL_OUT_OF_RESOURCES;
    }
    cl::retainInternal(event);
  }
  return CL_SUCCESS;
}

[[nodiscard]] cl_int _cl_command_queue::dispatch_state_t::addCallback(
    std::function<void()> callback) {
  if (callbacks.push_back(std::move(callback))) {
    return CL_OUT_OF_RESOURCES;
  }
  return CL_SUCCESS;
}

cl_int _cl_command_queue::finish_state_t::addState(
    cl_command_queue command_queue,
    cargo::small_vector<cl_event, 8> signal_events,
    cargo::small_vector<std::function<void()>, 8> callbacks) {
  // We do not require locking the command queue's mutex here because it is
  // always invoked from a member function which already holds the lock.
  this->command_queue = command_queue;
  this->signal_events = std::move(signal_events);
  this->callbacks = std::move(callbacks);
  return cargo::success;
}

void _cl_command_queue::finish_state_t::clear(
    mux_command_buffer_t command_buffer, mux_result_t error, bool locked) {
  const cl_int cl_error =
      (mux_success == error) ? CL_COMPLETE : CL_OUT_OF_RESOURCES;
  for (auto signal_event : signal_events) {
    signal_event->complete(cl_error);
    cl::releaseInternal(signal_event);
  }
  signal_events.clear();
  std::for_each(callbacks.rbegin(), callbacks.rend(),
                [](std::function<void()> &callback) { callback(); });
  callbacks.clear();
  if (locked) {
    command_queue->finish_state.erase(command_buffer);
  } else {
    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());
    command_queue->finish_state.erase(command_buffer);
  }
}

CL_API_ENTRY cl_command_queue CL_API_CALL cl::CreateCommandQueue(
    cl_context context, cl_device_id device_id,
    cl_command_queue_properties properties, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateCommandQueue");
  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  OCL_CHECK(!device_id || !context->hasDevice(device_id),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_DEVICE);
            return nullptr);

  auto queue = _cl_command_queue::create(context, device_id, properties);

  if (!queue) {
    OCL_SET_IF_NOT_NULL(errcode_ret, queue.error());
    return nullptr;
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return queue->release();
}

CL_API_ENTRY cl_int CL_API_CALL
cl::RetainCommandQueue(cl_command_queue command_queue) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clRetainCommandQueue");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  return cl::retainExternal(command_queue);
}

CL_API_ENTRY cl_int CL_API_CALL
cl::ReleaseCommandQueue(cl_command_queue command_queue) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clReleaseCommandQueue");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);

  // If we are on the last ref count external and there is still an internal
  // refcount, then flush and wait for events.
  if (command_queue->refCountExternal() == 1 &&
      command_queue->refCountInternal()) {
    command_queue->finish();
  } else {
    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    // releasing a command queue causes an implicit flush
    if (auto error = command_queue->flush()) {
      return error;
    }
  }
  return cl::releaseExternal(command_queue);
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetCommandQueueInfo(
    cl_command_queue command_queue, cl_command_queue_info param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetCommandQueueInfo");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);

#define COMMAND_QUEUE_INFO_CASE(TYPE, SIZE_RET, POINTER, VALUE)    \
  case TYPE: {                                                     \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, SIZE_RET);           \
    OCL_CHECK(param_value && (param_value_size < SIZE_RET),        \
              return CL_INVALID_VALUE);                            \
    OCL_SET_IF_NOT_NULL(static_cast<POINTER>(param_value), VALUE); \
  } break

  switch (param_name) {
    COMMAND_QUEUE_INFO_CASE(CL_QUEUE_CONTEXT, sizeof(_cl_context *),
                            _cl_context **, command_queue->context);
    COMMAND_QUEUE_INFO_CASE(CL_QUEUE_DEVICE, sizeof(_cl_device_id *),
                            _cl_device_id **, command_queue->device);
    COMMAND_QUEUE_INFO_CASE(CL_QUEUE_REFERENCE_COUNT, sizeof(cl_uint),
                            cl_uint *, command_queue->refCountExternal());
    COMMAND_QUEUE_INFO_CASE(
        CL_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties),
        cl_command_queue_properties *, command_queue->properties);
#if defined(CL_VERSION_3_0)
    COMMAND_QUEUE_INFO_CASE(CL_QUEUE_DEVICE_DEFAULT, sizeof(cl_command_queue),
                            cl_command_queue *, nullptr);
    case CL_QUEUE_SIZE:
      return CL_INVALID_COMMAND_QUEUE;
    case CL_QUEUE_PROPERTIES_ARRAY:
      if (command_queue->properties_list.empty()) {
        OCL_SET_IF_NOT_NULL(param_value_size_ret, 0);
      } else {
        OCL_SET_IF_NOT_NULL(
            param_value_size_ret,
            sizeof(cl_bitfield) * command_queue->properties_list.size());
        OCL_CHECK(param_value && param_value_size <
                                     sizeof(cl_bitfield) *
                                         command_queue->properties_list.size(),
                  return CL_INVALID_VALUE);
        if (param_value) {
          std::copy(command_queue->properties_list.begin(),
                    command_queue->properties_list.end(),
                    static_cast<cl_bitfield *>(param_value));
        }
      }
      break;
#endif
    default: {
      return extension::GetCommandQueueInfo(command_queue, param_name,
                                            param_value_size, param_value,
                                            param_value_size_ret);
    }
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueBarrierWithWaitList(
    cl_command_queue command_queue, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard(
      "clEnqueueBarrierWithWaitList");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);

  const cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  cl_event return_event = nullptr;
  if (nullptr != event) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_BARRIER);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
  }
  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  // barriers are implicit in in-order queues, could mostly be a no-op
  // (especially if we don't have a return event!) but we may have cross-queue
  // events to wait for
  auto command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  if (nullptr != event) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueMarkerWithWaitList(
    cl_command_queue command_queue, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueMarkerWithWaitList");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);

  const cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  cl_event return_event = nullptr;
  if (nullptr != event) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_MARKER);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
  }
  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  auto mux_command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  if (nullptr != event) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueWaitForEvents(
    cl_command_queue queue, cl_uint num_events, const cl_event *event_list) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueWaitForEvents");
  OCL_CHECK(!queue, return CL_INVALID_COMMAND_QUEUE);

  // This does not use the cl::validate::EventWaitList because this call is not
  // part of OpenCL 1.2 and validates the event wait list differently.
  OCL_CHECK((0 == num_events) || !(event_list), return CL_INVALID_VALUE);

  for (cl_uint i = 0; i < num_events; i++) {
    OCL_CHECK(!event_list[i], return CL_INVALID_EVENT);
    OCL_CHECK(event_list[i]->context != queue->context,
              return CL_INVALID_CONTEXT);
  }

#ifndef CA_ENABLE_OUT_OF_ORDER_EXEC_MODE
  OCL_CHECK(cl::validate::IsInBitSet(queue->properties,
                                     CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE),
            OCL_ABORT("OCL API objects event. Error clEnqueueWaitForEvents "
                      "does not support out of order execution"));
#endif

  const std::lock_guard<std::mutex> lock(
      queue->context->getCommandQueueMutex());

  auto mux_command_buffer =
      queue->getCommandBuffer({event_list, num_events}, nullptr);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::Flush(cl_command_queue command_queue) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clFlush");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());
  return command_queue->flush();
}

cl_int _cl_command_queue::finish() {
  {
    const std::lock_guard<std::mutex> lock(context->getCommandQueueMutex());
    flush();
  }

  if (mux_success != muxWaitAll(mux_queue)) {
    return CL_OUT_OF_RESOURCES;
  }

  {
    const std::lock_guard<std::mutex> lock(context->getCommandQueueMutex());
    if (CL_SUCCESS != cleanupCompletedCommandBuffers()) {
      return CL_OUT_OF_RESOURCES;
    }
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::Finish(cl_command_queue command_queue) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clFinish");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);

  auto new_event = _cl_event::create(command_queue, 0);
  if (!new_event) {
    return new_event.error();
  }
  cl::release_guard<cl_event> event_release_guard(*new_event,
                                                  cl::ref_count_type::EXTERNAL);
  auto finish_result = command_queue->finish();
  if (CL_SUCCESS != finish_result) {
    event_release_guard->complete(CL_OUT_OF_RESOURCES);
    return finish_result;
  }

  event_release_guard->complete();

  cl_int result;
  {
    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());
    result = command_queue->flush();
  }

  if (CL_SUCCESS != result) {
    return result;
  }

  command_queue->waitForEvents(1, &event_release_guard.get());

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueBarrier(cl_command_queue queue) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueBarrier");
  OCL_CHECK(!queue, return CL_INVALID_COMMAND_QUEUE);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::EnqueueMarker(cl_command_queue command_queue, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueMarker");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!event, return CL_INVALID_VALUE);

  // No-op if the user provided event is null
  if (nullptr != event) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_MARKER);
    if (!new_event) {
      return new_event.error();
    }
    *event = *new_event;

    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer({}, *event);
    if (!mux_command_buffer) {
      return CL_OUT_OF_RESOURCES;
    }
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::SetCommandQueueProperty(
    cl_command_queue command_queue, cl_command_queue_properties properties,
    cl_bool enable, cl_command_queue_properties *old_properties) {
  const tracer::TraceGuard<tracer::OpenCL> guard("SetCommandQueueProperty");
  // 	clSetCommandQueueProperty is deprecated by version 1.1.
  (void)command_queue;
  (void)properties;
  (void)enable;
  (void)old_properties;
  return CL_INVALID_OPERATION;
}

#ifdef OCL_EXTENSION_cl_khr_command_buffer
[[nodiscard]] cl_int _cl_command_queue::enqueueCommandBuffer(
    cl_command_buffer_khr command_buffer, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *return_event) {
  // Lock both queue and command-buffer
  const std::lock_guard<std::mutex> lock_queue(context->getCommandQueueMutex());
  const std::lock_guard<std::mutex> lock_command_buffer(command_buffer->mutex);

  // Create the signal event if caller asks for it.
  cl_event event = nullptr;
  if (nullptr != return_event) {
    auto new_event = _cl_event::create(this, CL_COMMAND_COMMAND_BUFFER_KHR);
    if (!new_event) {
      return new_event.error();
    }
    event = *new_event;
    *return_event = event;
  }

  // The queue will hold a reference on the cl_command_buffer_khr until its
  // dispatch is complete.
  cl::retainInternal(command_buffer);
  cl::release_guard<cl_command_buffer_khr> guard(command_buffer,
                                                 cl::ref_count_type::INTERNAL);
  // We need to acquire the lock on the command buffer since we will read and
  // modify its state throughout this function.

  auto mux_command_buffer = command_buffer->mux_command_buffer;

  // We need to check if the mux_command_buffer associated with the
  // cl_command_buffer_khr object has already been enqueued to a command
  // queue. If it has, then we need to clone the underlying mux_command_buffer_t
  // since mux_command_buffer_ts are single use.
  auto command_queue_should_destroy_command_buffer = false;
  if (command_buffer->execution_refcount > 0) {
    mux_command_buffer_t cloned_mux_command_buffer;
    auto device = command_buffer->command_queue->device;
    if (auto error = muxCloneCommandBuffer(
            device->mux_device, device->mux_allocator, mux_command_buffer,
            &cloned_mux_command_buffer)) {
      return cl::getErrorFrom(error);
    }
    mux_command_buffer = cloned_mux_command_buffer;
    command_queue_should_destroy_command_buffer = true;
  }

#ifdef OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch
  for (_cl_command_buffer_khr::UpdateInfo &update : command_buffer->updates) {
    const unsigned num_args = update.indices.size();
    auto mux_error =
        muxUpdateDescriptors(mux_command_buffer, update.id, num_args,
                             update.indices.data(), update.descriptors.data());
    if (mux_error) {
      return cl::getErrorFrom(mux_error);
    }
  }
  command_buffer->updates.clear();
#endif

  // Since we can't do any batching with user command buffers we can just wait
  // directly on the last pending dispatch (we need to do this anyway to enforce
  // an in order queue). Since the queue is in order, we know that any event
  // dependencies requested by the user will still be respected. This will not
  // work for cross queue event dependencies (see CA-3276).
  if (!pending_command_buffers.empty()) {
    auto &signal_semaphore =
        pending_dispatches[pending_command_buffers.back()].signal_semaphore;
    if (pending_dispatches[mux_command_buffer].wait_semaphores.push_back(
            signal_semaphore)) {
      return CL_OUT_OF_RESOURCES;
    } else {
      signal_semaphore->retain();
    }
  }

  // Add the underlying mux_command_buffer associated to the
  // cl_command_buffer_khr to the list of pending command buffers.
  if (pending_command_buffers.push_back(mux_command_buffer)) {
    // We won't destroy the mux_command_buffer here since that is the
    // responsibility of the command buffer.
    return CL_OUT_OF_HOST_MEMORY;
  }

  // Create a semaphore which future command buffers can wait for.
  auto semaphore = createSemaphore();
  if (!semaphore) {
    return semaphore.error();
  }

  // Add the signal semaphore and wait/signal events to the pending dispatch
  // object used to track this command buffer before it is dispatched.
  auto &dispatch = pending_dispatches[mux_command_buffer];
  dispatch.signal_semaphore = *semaphore;
  dispatch.is_user_command_buffer = true;
  dispatch.should_destroy_command_buffer =
      command_queue_should_destroy_command_buffer;

  if (auto error =
          dispatch.addWaitEvents({event_wait_list, num_events_in_wait_list})) {
    if (event) {
      event->complete(error);
    }
    return CL_OUT_OF_RESOURCES;
  }
  if (auto error = dispatch.addSignalEvent(event)) {
    if (event) {
      event->complete(error);
    }
    return CL_OUT_OF_RESOURCES;
  }

  // Add callbacks to all the user events in the wait list.
  for (unsigned i = 0; i < num_events_in_wait_list; ++i) {
    auto wait_event = event_wait_list[i];
    // Do not wait on completed commands.
    if (cl::isUserEvent(wait_event) &&
        wait_event->command_status != CL_COMPLETE) {
      if (!wait_event->addCallback(CL_COMPLETE, &userEventDispatch, this)) {
        return CL_OUT_OF_RESOURCES;
      }
    }
  }

  // We need to wait on all running commands to enforce ordering.
  for (auto &running_command_buffer : running_command_buffers) {
    if (pending_dispatches[mux_command_buffer].wait_semaphores.push_back(
            running_command_buffer.signal_semaphore)) {
      releaseSemaphore(*semaphore);
      return CL_OUT_OF_HOST_MEMORY;
    } else {
      running_command_buffer.signal_semaphore->retain();
    }
  }

  // Increment refcount so that command-buffer state moves to Pending
  command_buffer->execution_refcount++;

  // Release the reference once the dispatch completes.
  guard.dismiss();
  user_command_buffers[mux_command_buffer] = command_buffer;

  return CL_SUCCESS;
}
#endif
