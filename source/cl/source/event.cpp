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

#include <cl/command_queue.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/event.h>
#include <cl/macros.h>
#include <cl/mux.h>
#include <cl/validate.h>
#include <tracer/tracer.h>
#include <utils/system.h>

cargo::expected<cl_event, cl_int> _cl_event::create(
    cl_command_queue queue, const cl_command_type type) {
  OCL_ASSERT(queue != nullptr, "queue must not be null");
  std::unique_ptr<_cl_event> event(new _cl_event(queue->context, queue, type));
  if (!event) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  if (queue->properties & CL_QUEUE_PROFILING_ENABLE) {
    event->profiling.enabled = true;
    // The lifetime of mux queues is not controllable so this is safe
    event->profiling.mux_queue = queue->mux_queue;
    event->profiling.mux_allocator = queue->device->mux_allocator;
    event->profiling.queued = utils::timestampNanoSeconds();
    if (auto mux_error = muxCreateQueryPool(
            queue->mux_queue, mux_query_type_duration, 1, nullptr,
            queue->device->mux_allocator, &event->profiling.duration_queries)) {
      OCL_ASSERT(mux_error != mux_error_invalid_value &&
                     mux_error != mux_error_null_out_parameter,
                 "internal error calling muxCreateQueryPool");
      OCL_UNUSED(mux_error);
      return cargo::make_unexpected(CL_OUT_OF_RESOURCES);
    }
  }
  return event.release();
}

cargo::expected<cl_event, cl_int> _cl_event::create(cl_context context) {
  std::unique_ptr<_cl_event> event(
      new _cl_event(context, nullptr, CL_COMMAND_USER));
  if (!event) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  return event.release();
}

_cl_event::_cl_event(cl_context context, cl_command_queue queue,
                     cl_command_type type)
    : base<_cl_event>(cl::ref_count_type::EXTERNAL),
      context(context),
      queue(queue),
      command_type(type),
      command_status(CL_QUEUED) {
  if (queue) {
    cl::retainInternal(queue);
  }
  cl::retainInternal(context);
}

_cl_event::~_cl_event() {
  clear();
  if (profiling.mux_queue && profiling.duration_queries) {
    muxDestroyQueryPool(profiling.mux_queue, profiling.duration_queries,
                        profiling.mux_allocator);
  }
  if (queue) {
    cl::releaseInternal(queue);
  }
  cl::releaseInternal(context);
}

bool _cl_event::addCallback(const cl_int type,
                            cl::pfn_event_notify_t pfn_event_notify,
                            void *user_data) {
  std::unique_lock<std::mutex> callback_lock(callback_mutex);
  const cl_int status = command_status;
  if (status <= type) {
    // The callback trigger already occurred. As another thread might be in the
    // process of calling and removing triggered callbacks from the event do not
    // interact with the callback container but call the callback function
    // directly.

    // Callbacks are allowed to interact with their event, e.g., set additional
    // callbacks, which might require a lock. Unlock to prevent a deadlock.
    callback_lock.unlock();
    pfn_event_notify(this, status, user_data);
  } else {
    if (cargo::success !=
        callbacks.push_back({type, pfn_event_notify, user_data})) {
      return false;
    }
  }
  return true;
}

void _cl_event::submitted() {
  if (profiling.enabled) {
    profiling.submit = utils::timestampNanoSeconds();
  }
  command_status = CL_SUBMITTED;
}

void _cl_event::running() { command_status = CL_RUNNING; }

void _cl_event::complete(const cl_int status) {
  const std::unique_lock<std::mutex> signal_lock(wait_complete_mutex);

  command_status = status;

  // Trigger callbacks before notifying condition variable.
  // This is not mandated by the OpenCL 1.2 specs but seems the correct order.
  clear();

  wait_complete_condition.notify_all();
}

void _cl_event::wait() {
  std::unique_lock<std::mutex> signal_lock(wait_complete_mutex);
  while (CL_COMPLETE < command_status) {
    wait_complete_condition.wait(signal_lock);
  }
}

void _cl_event::clear() {
  OCL_ASSERT(cl::isUserEvent(this) || CL_COMPLETE >= command_status.load(),
             "Function removes all callbacks regardless of if their status has "
             "been reached or surpassed. Only call for completed events or "
             "user events.");

  // Lock to guarantee up-to-date view of callbacks and prevent status updates
  std::unique_lock<std::mutex> callback_lock(callback_mutex);

  // Function should be called with a complete or error status. No further
  // status changes are expected, so read the atomic value once and reuse it
  // below.
  const cl_int status = command_status;

  // As callbacks can add more callbacks to an event, always re-check the
  // container.
  // Users can live-lock the thread this way...
  while (!callbacks.empty()) {
    // Work on batches of callbacks to minimize unlocking and locking.
    // Assuming that all callbacks have completed or failed.
    const cargo::small_vector<callback_state_t, 4> callbacks_to_execute =
        std::move(callbacks);
    // Ensure that callbacks are not called more than once.
    callbacks.clear();

    // Callbacks may interact with the event and require locking. Unlock the
    // event to prevent deadlocks.
    callback_lock.unlock();
    {
      for (auto &callback : callbacks_to_execute) {
        callback.pfn_event_notify(this, status, callback.user_data);
      }
    }
    // Re-lock before re-checking the callbacks in the loop condition.
    callback_lock.lock();
  }
}

CL_API_ENTRY cl_event CL_API_CALL cl::CreateUserEvent(cl_context context,
                                                      cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateUserEvent");
  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  auto new_event = _cl_event::create(context);
  if (!new_event) {
    OCL_SET_IF_NOT_NULL(errcode_ret, new_event.error());
    return nullptr;
  }
  cl_event event = *new_event;
  event->submitted();
  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return event;
}

CL_API_ENTRY cl_int CL_API_CALL cl::RetainEvent(cl_event event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clRetainEvent");
  OCL_CHECK(!event, return CL_INVALID_EVENT);
  return cl::retainExternal(event);
}

CL_API_ENTRY cl_int CL_API_CALL cl::ReleaseEvent(cl_event event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clReleaseEvent");
  OCL_CHECK(!event, return CL_INVALID_EVENT);
  return cl::releaseExternal(event);
}

CL_API_ENTRY cl_int CL_API_CALL cl::WaitForEvents(cl_uint num_events,
                                                  const cl_event *event_list) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clWaitForEvents");
  OCL_CHECK(0 == num_events, return CL_INVALID_VALUE);
  OCL_CHECK(!event_list, return CL_INVALID_VALUE);

  _cl_context *previousContext = nullptr;
  _cl_command_queue *previousQueue = nullptr;
  bool moreThanOneQueue = false;
  bool userEventInList = false;

  for (cl_uint i = 0; i < num_events; i++) {
    OCL_CHECK(!(event_list[i]), return CL_INVALID_EVENT);
    _cl_context *currentContext = event_list[i]->context;
    OCL_CHECK(!currentContext, return CL_INVALID_CONTEXT);
    OCL_CHECK(previousContext && (previousContext != currentContext),
              return CL_INVALID_CONTEXT);
    previousContext = currentContext;
    if (!moreThanOneQueue) {
      if (nullptr == previousQueue) {
        previousQueue = event_list[i]->queue;
      } else if (nullptr != event_list[i]->queue) {
        if (event_list[i]->queue != previousQueue) {
          moreThanOneQueue = true;
        }
      }
    }

    if (CL_COMMAND_USER == event_list[i]->command_type) {
      userEventInList = true;
    }
  }

  // need to implicitly flush all the queues that the events belong to.
  for (cl_uint i = 0; i < num_events; i++) {
    // if the event belonged to a queue
    if (nullptr != event_list[i]->queue) {
      const std::lock_guard<std::mutex> lock(
          event_list[i]->queue->context->getCommandQueueMutex());
      const cl_int result = event_list[i]->queue->flush();

      if (CL_SUCCESS != result) {
        return result;
      }

      if (!moreThanOneQueue) {
        // if we had exactly one queue, we don't need to re-flush the same queue
        // multiple times
        break;
      }
    }
  }

  // if we are waiting on more than one queue we have to wait on each event
  // separately
  if (moreThanOneQueue || userEventInList) {
    for (cl_uint i = 0; i < num_events; i++) {
      // if the event did not belong to a queue
      if (nullptr == event_list[i]->queue) {
        event_list[i]->wait();
      } else {
        event_list[i]->queue->waitForEvents(1, &event_list[i]);
      }
    }
  } else {
    for (cl_uint i = 0; i < num_events; i++) {
      if (nullptr == event_list[i]->queue) {
        continue;
      }

      // if the event belonged to a queue, query the queue to wait for the event
      event_list[i]->queue->waitForEvents(num_events, event_list);

      break;
    }
  }

  for (cl_uint i = 0; i < num_events; i++) {
    OCL_CHECK(0 > (event_list[i]->command_status),
              return CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetEventInfo(cl_event event,
                                                 cl_event_info param_name,
                                                 size_t param_value_size,
                                                 void *param_value,
                                                 size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetEventInfo");
  OCL_CHECK(!event, return CL_INVALID_EVENT);

  if (CL_EVENT_COMMAND_EXECUTION_STATUS == param_name) {
    if (nullptr != param_value) {
      if (param_value_size < sizeof(cl_int)) {
        return CL_INVALID_VALUE;
      }

      // if we have a queue, we query that for the event status
      *static_cast<cl_int *>(param_value) =
          (nullptr != event->queue) ? event->queue->getEventStatus(event)
                                    : (cl_int)event->command_status;
    }

    if (nullptr != param_value_size_ret) {
      *param_value_size_ret = sizeof(cl_int);
    }

  } else {
#define EVENT_INFO_CASE(NAME, RETURN_TYPE, VALUE)                        \
  case NAME: {                                                           \
    OCL_CHECK(param_value &&param_value_size < sizeof(RETURN_TYPE),      \
              return CL_INVALID_VALUE);                                  \
    OCL_SET_IF_NOT_NULL(static_cast<RETURN_TYPE *>(param_value), VALUE); \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(RETURN_TYPE));      \
  } break

    switch (param_name) {
      EVENT_INFO_CASE(CL_EVENT_COMMAND_QUEUE, _cl_command_queue *,
                      event->queue);
      EVENT_INFO_CASE(CL_EVENT_CONTEXT, _cl_context *, event->context);
      EVENT_INFO_CASE(CL_EVENT_COMMAND_TYPE, cl_int, event->command_type);
      EVENT_INFO_CASE(CL_EVENT_REFERENCE_COUNT, cl_uint,
                      event->refCountExternal());
      default: {
        return extension::GetEventInfo(event, param_name, param_value_size,
                                       param_value, param_value_size_ret);
      }
    }
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::SetEventCallback(cl_event event, cl_int command_exec_callback_type,
                     cl::pfn_event_notify_t pfn_event_notify, void *user_data) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clSetEventCallback");
  OCL_CHECK(!event, return CL_INVALID_EVENT);
  OCL_CHECK(!pfn_event_notify, return CL_INVALID_VALUE);

  OCL_CHECK(!(CL_SUBMITTED == command_exec_callback_type ||
              CL_RUNNING == command_exec_callback_type ||
              CL_COMPLETE == command_exec_callback_type),
            return CL_INVALID_VALUE);

  const bool success = event->addCallback(command_exec_callback_type,
                                          pfn_event_notify, user_data);
  OCL_CHECK(!success, return CL_OUT_OF_HOST_MEMORY);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::SetUserEventStatus(cl_event event, cl_int execution_status) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clSetUserEventStatus");
  OCL_CHECK(!event, return CL_INVALID_EVENT);
  OCL_CHECK(CL_COMMAND_USER != event->command_type, return CL_INVALID_EVENT);
  OCL_CHECK(CL_COMPLETE < execution_status, return CL_INVALID_VALUE);
  OCL_CHECK(CL_COMPLETE >= event->command_status, return CL_INVALID_OPERATION);

  // Need to retain the event as otherwise a malicious programmer could have
  // added a callback to the event that freed itself, which results in the
  // following command very rarely touching freed memory.
  cl::retainInternal(event);
  event->complete(execution_status);
  cl::releaseInternal(event);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetEventProfilingInfo(
    cl_event event, cl_profiling_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetEventProfilingInfo");
  OCL_CHECK(!event, return CL_INVALID_EVENT);
  OCL_CHECK(CL_COMMAND_USER == event->command_type,
            return CL_PROFILING_INFO_NOT_AVAILABLE);
  OCL_CHECK(!cl::validate::IsInBitSet(event->queue->properties,
                                      CL_QUEUE_PROFILING_ENABLE) &&
                !event->queue->counter_queries,
            return CL_PROFILING_INFO_NOT_AVAILABLE);

  OCL_CHECK(param_value && param_value_size < sizeof(cl_ulong),
            return CL_INVALID_VALUE);
  OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_ulong));

  // If 'param_name' is not a legal value we should return CL_INVALID_VALUE.
  switch (param_name) {
    case CL_PROFILING_COMMAND_QUEUED:
    case CL_PROFILING_COMMAND_SUBMIT:
    case CL_PROFILING_COMMAND_START:
    case CL_PROFILING_COMMAND_END:
#if defined(CL_VERSION_3_0)
    case CL_PROFILING_COMMAND_COMPLETE:
#endif
      break;  // The above are all legal parameter names.
    default:
      // However, an extension may support additional names so we also need to
      // check the extension interface.  However, we don't yet know if we
      // should expect any extension counters to be set as we haven't yet
      // checked the event status, so we don't pass along 'param_value', just
      // pass nullptr.  We should check this *before* checking the event status
      // as illegal parameters are always illegal, and thus that error code is
      // consistent (but whether an event is complete yet or not is inherently
      // unstable).
      OCL_CHECK(CL_INVALID_VALUE == extension::GetEventProfilingInfo(
                                        event, param_name, param_value_size,
                                        param_value, param_value_size_ret),
                return CL_INVALID_VALUE);
  }

  // OpenCL 1.2 specification says that profiling event information is not
  // queryable until an event's status is CL_COMPLETE.  This also means that if
  // the status is CL_COMPLETE we can read the profiling values without locking
  // 'wait_complete_mutex' because the profiling values get set before the
  // status is set.
  OCL_CHECK(CL_COMPLETE != event->command_status,
            return CL_PROFILING_INFO_NOT_AVAILABLE);

  switch (param_name) {
    case CL_PROFILING_COMMAND_QUEUED:
      OCL_SET_IF_NOT_NULL(
          reinterpret_cast<cl_ulong *>(param_value),
          event->profiling.queued - event->queue->profiling_start);
      break;
    case CL_PROFILING_COMMAND_SUBMIT:
      OCL_SET_IF_NOT_NULL(
          reinterpret_cast<cl_ulong *>(param_value),
          event->profiling.submit - event->queue->profiling_start);
      break;
    case CL_PROFILING_COMMAND_START: {
      mux_query_duration_result_s duration;
      if (auto mux_error = muxGetQueryPoolResults(
              event->queue->mux_queue, event->profiling.duration_queries, 0, 1,
              sizeof(mux_query_duration_result_s), &duration,
              sizeof(mux_query_duration_result_s))) {
        return cl::getErrorFrom(mux_error);
      }
      OCL_SET_IF_NOT_NULL(reinterpret_cast<cl_ulong *>(param_value),
                          duration.start - event->queue->profiling_start);
    } break;
#if defined(CL_VERSION_3_0)
    case CL_PROFILING_COMMAND_COMPLETE:
      // Returns a value equivalent to passing CL_PROFILING_COMMAND_END
      // if the device associated with event does not support On-Device Enqueue.
      [[fallthrough]];
#endif
    case CL_PROFILING_COMMAND_END: {
      mux_query_duration_result_s duration;
      if (auto mux_error = muxGetQueryPoolResults(
              event->queue->mux_queue, event->profiling.duration_queries, 0, 1,
              sizeof(mux_query_duration_result_s), &duration,
              sizeof(mux_query_duration_result_s))) {
        return cl::getErrorFrom(mux_error);
      }
      OCL_SET_IF_NOT_NULL(reinterpret_cast<cl_ulong *>(param_value),
                          duration.end - event->queue->profiling_start);
    } break;
    default:
      return extension::GetEventProfilingInfo(event, param_name,
                                              param_value_size, param_value,
                                              param_value_size_ret);
  }

  return CL_SUCCESS;
}
