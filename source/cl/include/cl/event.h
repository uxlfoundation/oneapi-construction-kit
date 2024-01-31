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

/// @file
///
/// @brief Definitions of the OpenCL event API.

#ifndef CL_EVENT_H_INCLUDED
#define CL_EVENT_H_INCLUDED

#include <CL/cl.h>
#include <cargo/expected.h>
#include <cargo/small_vector.h>
#include <cl/base.h>
#include <cl/config.h>
#include <cl/limits.h>
#include <mux/mux.h>

#include <condition_variable>
#include <mutex>

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Event callback function pointer definition.
///
/// @param[in] event Event the callback belongs to.
/// @param[in] event_command_exec_status Static of the commands execution.
/// @param[in] user_data User data to be passed to the callback function.
using pfn_event_notify_t = void(CL_CALLBACK *)(cl_event event,
                                               cl_int event_command_exec_status,
                                               void *user_data);

/// @}
}  // namespace cl

/// @addtogroup cl
/// @{

/// @brief Definition of OpenCL API event object.
struct _cl_event final : public cl::base<_cl_event> {
  /// @brief Event callback container.
  struct callback_state_t {
    /// @brief Type of command the event relates to.
    cl_int type;
    /// @brief Callback function to invoke when execution status changes.
    cl::pfn_event_notify_t pfn_event_notify;
    /// @brief User data to be passed to the callback function.
    void *user_data;
  };

  /// @brief Profiling data container.
  struct profiling_state_t {
    /// @brief Time when the command was added to the queue.
    cl_ulong queued = 0;
    /// @brief Time when the command was submitted for execution.
    cl_ulong submit = 0;
    /// @brief Mux query pool for storing command duration query results.
    mux_query_pool_t duration_queries = nullptr;
    /// @brief associated queue against which profiling queries are made
    mux_queue_t mux_queue = nullptr;
    /// @brief the mux allocator that handles the queue
    mux_allocator_info_t mux_allocator = {};
    /// @brief Is profiling enabled or not.
    bool enabled = false;
  };

  /// @brief Create an command event.
  ///
  /// @param queue The command queue the event belongs to.
  /// @param type The type of command the event represents.
  ///
  /// @return Returns the event object on success.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failed.
  /// @retval `CL_OUT_OF_RESOURCES` if profiling could not be enabled due to an
  /// error in `muxCreateQueryPool`.
  static cargo::expected<cl_event, cl_int> create(cl_command_queue queue,
                                                  const cl_command_type type);

  /// @brief Create a user event.
  ///
  /// @param context The context the event belongs to.
  ///
  /// @return Returns the event object on success.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failed.
  static cargo::expected<cl_event, cl_int> create(cl_context context);

  /// @brief Destructor.
  ~_cl_event();

  /// @brief Register a notification callback function to the event.
  ///
  /// @param[in] type Type of command the event is tracking.
  /// @param[in] pfn_event_notify Event notification callback function pointer.
  /// @param[in] user_data User data to be passed to the callback function.
  ///
  /// @return Return true on success, false otherwise.
  bool addCallback(const cl_int type, cl::pfn_event_notify_t pfn_event_notify,
                   void *user_data);

  /// @brief Signal that the event's command has been submitted for execution.
  void submitted();

  /// @brief Signal that the event's command is currently being executed.
  void running();

  /// @brief Signal that the event's command has completed execution.
  ///
  /// @param[in] status Execution status of the command.
  void complete(const cl_int status = CL_COMPLETE);

  /// @brief Wait for the event to complete execution.
  void wait();

  /// @brief Context the event belongs to.
  cl_context context;
  /// @brief Command queue the event belongs to.
  cl_command_queue queue;
  /// @brief Type of command the event relates to.
  cl_command_type command_type;
  /// @brief The current execution status of the event's command.
  std::atomic<cl_int> command_status;
  /// @brief Profiling data container.
  profiling_state_t profiling;

 private:
  /// @brief Event constructor.
  ///
  /// @param[in] context Context the event belongs to.
  /// @param[in] queue Command queue the event is submitted on.
  /// @param[in] type Type of command the event is tracking.
  _cl_event(cl_context context, cl_command_queue queue,
            const cl_command_type type);

  _cl_event(const _cl_event &) = delete;
  _cl_event &operator=(const _cl_event &) = delete;

  /// @brief Remove all registered callbacks and call them.
  ///
  /// Function removes all callbacks regardless of if their status has been
  /// reached or surpassed. Only call for completed events or user events.
  void clear();

  /// @brief Mutex used for signalling between _cl_event::wait() and
  /// _cl_event::complete() member functions.
  std::mutex wait_complete_mutex;
  /// @brief Condition variable used for signalling between _cl_event::wait()
  /// and _cl_event::complete() member functions.
  std::condition_variable wait_complete_condition;
  /// @brief Mutex to protect concurrent access to _cl_event::callbacks.
  ///
  /// The mutex needs to be recursive, as nothing prohibits a callback from
  /// calling clSetEventCallback on the event it is operating on.
  std::mutex callback_mutex;
  /// @brief Vector of registered event callback functions.
  cargo::small_vector<callback_state_t, 4> callbacks;
};

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Check if an event is a user event.
///
/// @param event Event to check.
///
/// @return Returns `true` if this event is a user event, `false` otherwise.
inline bool isUserEvent(const cl_event event) {
  return CL_COMMAND_USER == event->command_type;
}

/// @brief Create an OpenCL user event object.
///
/// @param[in] context Context the user event belong to.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new user event object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateUserEvent.html
CL_API_ENTRY cl_event CL_API_CALL CreateUserEvent(cl_context context,
                                                  cl_int *errcode_ret);

/// @brief Increment the events reference count.
///
/// @param[in] event Event to increment the reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clRetainEvent.html
CL_API_ENTRY cl_int CL_API_CALL RetainEvent(cl_event event);

/// @brief Decrement the event reference count.
///
/// @param[in] event Event to decrement the reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clReleaseEvent.html
CL_API_ENTRY cl_int CL_API_CALL ReleaseEvent(cl_event event);

/// @brief Wait for a list of events to complete execution.
///
/// @param[in] num_events Number of events in list to wait for.
/// @param[in] event_list List of events to wait for.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clWaitForEvents.html
CL_API_ENTRY cl_int CL_API_CALL WaitForEvents(cl_uint num_events,
                                              const cl_event *event_list);

/// @brief Query the event for information.
///
/// @param[in] event Event to query for information.
/// @param[in] param_name Type of information to query.
/// @param[in] param_value_size Size in bytes of `param_value` storage.
/// @param[in] param_value Pointer to value to store query in.
/// @param[out] param_value_size_ret Return size in bytes the query requires if
/// not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetEventInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetEventInfo(cl_event event,
                                             cl_event_info param_name,
                                             size_t param_value_size,
                                             void *param_value,
                                             size_t *param_value_size_ret);

/// @brief Register a callback function to notify when execution status changes.
///
/// @param[in] event Event to register callback with.
/// @param[in] command_exec_callback_type The command execution status the
/// callback is registered for.
/// @param[in] pfn_event_notify Callback function to invoke when the event is
/// triggered.
/// @param[in] user_data User data to pass to the callback function.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clSetEventCallback.html
CL_API_ENTRY cl_int CL_API_CALL
SetEventCallback(cl_event event, cl_int command_exec_callback_type,
                 cl::pfn_event_notify_t pfn_event_notify, void *user_data);

/// @brief Set the user events execution status.
///
/// @param[in] event User event to set the execution status on.
/// @param[in] execution_status Execution status to set on the user event.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clSetUserEventStatus.html
CL_API_ENTRY cl_int CL_API_CALL SetUserEventStatus(cl_event event,
                                                   cl_int execution_status);

/// @brief Query the event for profiling information.
///
/// @param[in] event Event to query profiling information from.
/// @param[in] param_name Type of profiling information to query.
/// @param[in] param_value_size Size in bytes of `param_value` storage.
/// @param[in] param_value Pointer to value to store profiling information in.
/// @param[out] param_value_size_ret Return size in bytes `param_value`
/// requires if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetEventProfilingInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetEventProfilingInfo(
    cl_event event, cl_profiling_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret);

/// @}
}  // namespace cl

#endif  // CL_EVENT_H_INCLUDED
