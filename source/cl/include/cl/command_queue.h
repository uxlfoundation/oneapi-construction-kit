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
/// @brief Definitions for the OpenCL command queue API.

#ifndef CL_COMMAND_QUEUE_H_INCLUDED
#define CL_COMMAND_QUEUE_H_INCLUDED

#include <CL/cl.h>
#include <cargo/array_view.h>
#include <cargo/expected.h>
#include <cargo/ring_buffer.h>
#include <cargo/small_vector.h>
#include <cl/base.h>
#include <cl/semaphore.h>
#ifdef OCL_EXTENSION_cl_khr_command_buffer
#include <extension/khr_command_buffer.h>
#endif
#include <mux/mux.h>

#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <tuple>
#include <unordered_map>
#include <vector>

/// @addtogroup cl
/// @{

/// @brief Definition of the OpenCL command queue object.
struct _cl_command_queue final : public cl::base<_cl_command_queue> {
 private:
  /// @brief Private constructor, use the `create()` functions instead.
  ///
  /// @param context Context the command queue belongs so.
  /// @param device Device the command queue targets.
  /// @param properties Command queue properties to be enabled.
  /// @param mux_queue Mux queue to execute work on.
  _cl_command_queue(cl_context context, cl_device_id device,
                    cl_command_queue_properties properties,
                    mux_queue_t mux_queue);

 public:
  /// @brief Destructor.
  ~_cl_command_queue();

  /// @brief Create command queue.
  ///
  /// @param context Context the command queue will belong to.
  /// @param device Device the command queue will target.
  /// @param properties Properties bitfield encoding which properties to enable.
  static cargo::expected<std::unique_ptr<_cl_command_queue>, cl_int> create(
      cl_context context, cl_device_id device,
      cl_command_queue_properties properties);

  /// @brief Create command queue with properties list.
  ///
  /// @param context Context the command queue will belong to.
  /// @param device Device the command queue will target.
  /// @param properties List of properties values denoting property information
  /// about the command queue to be created.
  static cargo::expected<std::unique_ptr<_cl_command_queue>, cl_int> create(
      cl_context context, cl_device_id device, const cl_bitfield *properties);

  /// @brief Flush the command queue.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  /// @return Returns an OpenCL error code.
  /// @retval `CL_SUCCESS` if there are no failures.
  /// @retval `CL_OUT_OF_RESOURCES` if destroying a resource fails.
  cl_int flush();

  /// @brief Wait for a series of events previously pushed to this queue.
  ///
  /// @param num_events The number of events in @p events.
  /// @param events An array of events to wait on.
  ///
  /// @return Returns an OpenCL error code.
  /// @retval `CL_SUCCESS` if there are no failures.
  /// @retval `CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST` if an error
  /// occurred waiting for any event in the wait list.
  cl_int waitForEvents(const cl_uint num_events, const cl_event *const events);

  /// @brief Query an event's status.
  ///
  /// @param event An event associated with the queue.
  ///
  /// @return The status of the event.
  cl_int getEventStatus(cl_event event);

  /// @brief Cleanup completed command buffers and signal semaphores.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  ///
  /// Implements a cleanup algorithm which checks for command buffer dispatch
  /// completion, when a completed dispatch is found; the command buffer is
  /// destroyed; the dispatches signal semaphore is removed from pending
  /// dispatches; if the signal semaphore is not being waited upon by a running
  /// command buffer dispatch, it is destroyed; otherwise it is added to
  /// `_cl_command_queue::completed_signal_semaphores` for later cleanup.
  ///
  /// @return OpenCL error code.
  cl_int cleanupCompletedCommandBuffers();

  /// @brief Get the device index of the command queue's device in the context.
  ///
  /// @return Returns index of the command queue's device in the context.
  cl_uint getDeviceIndex();

  /// @brief Get a command buffer to push commands onto.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  ///
  /// Calls to `getCommandBuffer()` are usually paired with calls to
  /// `registerDispatchCallback()` so that associated resources can be cleaned
  /// up when the command buffer dispatch is completed. These pairs of calls
  /// **must** be performed under the same mutex lock otherwise the command
  /// queue will be in an inconsistent state which may result in memory leaks
  /// or threading issues.
  ///
  /// @param event_wait_list List of events to wait on.
  /// @param event Return event the dispatch sets status of.
  ///
  /// @return Returns the expected command buffer or `CL_OUT_OF_RESOURCES`.
  [[nodiscard]] cargo::expected<mux_command_buffer_t, cl_int> getCommandBuffer(
      cargo::array_view<const cl_event> event_wait_list, cl_event event);

  /// @brief Register a command buffer dispatch completion callback.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  ///
  /// Calls to `registerDispatchCallback()` are usually paired with calls to
  /// `getCommandBuffer()` so that associated resources can be cleaned up when
  /// the command buffer dispatch is completed. These pairs of calls **must** be
  /// performed under the same mutex lock otherwise the command queue will be
  /// in an inconsistent state which may result in memory leaks or threading
  /// issues.
  ///
  /// @param command_buffer Command buffer to dispatch.
  /// @param event Return event to use for profiling results, when enabled.
  /// @param callback Callback to be called on completion.
  ///
  /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
  [[nodiscard]] cl_int registerDispatchCallback(
      mux_command_buffer_t command_buffer, cl_event event,
      std::function<void()> callback);

  /// @brief Flush and wait for any outstanding events
  ///
  ///
  /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
  cl_int finish();

#ifdef OCL_EXTENSION_cl_khr_command_buffer
  /// @brief Enqueue a command group from a cl_command_buffer_khr.
  ///
  /// Enqueing mux_command_buffers from cl_command_buffers_khr needs to be
  /// handled separately to:
  /// 1. Avoid destroying them at the end of their execution
  /// 2. Avoid enqueuing subsequent non khr_command_buffer related commands into
  /// the underlying mux_command_buffer associated with the
  /// cl_command_buffer_khr.
  ///
  /// @param[in] command_buffer the cl_command_buffer_khr to enqueue.
  /// @param[in] num_events_in_wait_list Number of event in the @p
  /// event_wait_list.
  /// @param[in] event_wait_list Events which must complete before
  /// command_buffer.
  /// @param[out] return_event Event which should be completed when
  /// command_buffer completes.
  ///
  /// @return Returns `CL_SUCCESS` if successful otherwise an appropriate OpenCL
  /// error code.
  [[nodiscard]] cl_int enqueueCommandBuffer(
      cl_command_buffer_khr command_buffer, cl_uint num_events_in_wait_list,
      const cl_event *event_wait_list, cl_event *return_event);

#endif

  /// @brief Context the command queue belongs to.
  cl_context context;
  /// @brief Device the command queue targets.
  cl_device_id device;

  /// @brief Properties enabled when the command queue was created.
  cl_command_queue_properties properties;

#if defined(CL_VERSION_3_0)
  /// @brief Array of properties values passed into
  /// `clCreateCommandQueueWithProperties`.
  cargo::small_vector<cl_queue_properties, 3> properties_list;
#endif

  /// @brief Command queue profiling epoch time.
  cl_ulong profiling_start;
  /// @brief Mux queue to execute work on.
  mux_queue_t mux_queue;
  /// @brief Mux query pool for storing performance counter results.
  mux_query_pool_t counter_queries;

 private:
  /// @brief Get the current command buffer, or create one if none exists.
  ///
  /// When `pending_command_buffers` is empty a new command buffer is created,
  /// otherwise the command buffer from the back of `pending_command_buffers` is
  /// used.
  ///
  /// @return Returns the expected command buffer or `CL_OUT_OF_RESOURCES`.
  [[nodiscard]] cargo::expected<mux_command_buffer_t, cl_int>
  getCurrentCommandBuffer();

  /// @brief Get a command buffer suitable for the given wait events.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  ///
  /// Performs the bulk of logic to determine if batching multiple commands in
  /// a command buffer is possible based on the `event_wait_list`, there are a
  /// number of possible cases:
  ///
  /// 1. There are no wait events and the last pending dispatch is not
  ///    associated with a user command buffer, get the current command
  ///    buffer or the last pending dispatch.
  /// 2. There are no wait events and the last pending dispatch is associated
  ///    with a user command buffer, get an unused (new or reset and cached)
  ///    command buffer.
  /// 3. There are wait events associated with a single pending dispatch, get
  ///    the associated command buffer.
  /// 4. There are wait events associated with multiple pending dispatches, get
  ///    an unused (new or reset and cached) command buffer.
  /// 5. There are wait events with no associated pending dispatches (already
  ///    dispatched), get an unused (new or reset and cached) command buffer.
  /// 6. There are wait events associated with a different queue.
  ///
  /// When commands can't be batched into a single command group, semaphores are
  /// used to maintain submission order since the command queue is in-order.
  /// Dispatches will have wait semaphores in the following cases:
  ///
  /// 1.  There are no wait events and there is a running command buffer.
  /// 2.  There are no wait events and the last pending dispatch is associated
  ///     with a user command buffer.
  /// 3.  There are wait events associated with multiple command buffers.
  /// 4.  There are wait events, no pending dispatches, and there is a running
  ///     command buffer.
  /// 5.  The wait event is cross queue.
  ///
  /// A user event completion callback is registered with all user events, it
  /// submits pending dispatches to the queue when the user event is in a
  /// success state, and removes them when in a failure state.
  ///
  /// @param event_wait_list List of events to wait for.
  ///
  /// @return Returns the expected command buffer or `CL_OUT_OF_RESOURCES`.
  [[nodiscard]] cargo::expected<mux_command_buffer_t, cl_int>
  getCommandBufferPending(cargo::array_view<const cl_event> event_wait_list);

  /// @brief Dispatch the given command buffers.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  ///
  /// If a command buffer depends on a user event it will not be dispatched by
  /// this member function, instead it will be dispatched when the user event
  /// enters the `CL_COMPLETE` status and calls `dispatchPending()` in the
  /// completion callback.
  ///
  /// @param[in] command_buffers List of command buffers to dispatch.
  ///
  /// @return Returns `CL_SUCCESS`, `CL_OUT_OF_RESOURCES`,
  /// `CL_OUT_OF_HOST_MEMORY` or `CL_INVALID_COMMAND_QUEUE`.
  [[nodiscard]] cl_int dispatch(
      cargo::array_view<mux_command_buffer_t> command_buffers);

  /// @brief Remove command buffers no longer pending dispatch.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  ///
  /// Modifies `pending_command_buffers` and `pending_dispatches` removing all
  /// references to `command_buffers`. Pointers to command buffer are not
  /// dereferenced, they are only used as lookup values.
  ///
  /// @param[in] command_buffers List of command buffers no longer pending
  /// dispatch.
  ///
  /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
  [[nodiscard]] cl_int removeFromPending(
      cargo::array_view<mux_command_buffer_t> command_buffers);

  /// @brief Dispatch command buffers associated with a user event.
  ///
  /// @note This member function is thread-safe, callers **must not** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  ///
  /// Intended for use in a user event callback when it becomes complete with a
  /// success status.
  ///
  /// @param user_event User event which has successfully completed.
  ///
  /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
  cl_int dispatchPending(cl_event user_event);

  /// @brief Drop dispatches depending on failed user events from pending.
  ///
  /// @note This member function is thread-safe, callers **must not** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  ///
  /// Intended for use in a user event callback when it becomes complete with
  /// an error status. Associated resources are released or destroyed,
  /// completion callbacks are invoked, and containers are cleared.
  ///
  /// @param user_event User events which have completed with a failure status.
  /// @param event_command_exec_status The failure status.
  ///
  /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
  cl_int dropDispatchesPending(cl_event user_event,
                               cl_int event_command_exec_status);

  /// @brief Create or get a cached command buffer.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  ///
  /// @return Returns the expected command buffer or `CL_OUT_OF_RESOURCES`.
  [[nodiscard]] cargo::expected<mux_command_buffer_t, cl_int>
  createCommandBuffer();

  /// @brief Cache or destroy a command buffer.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `_cl_command_queue->mutex` when calling it.
  ///
  /// @param command_buffer a mux command buffer.
  /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
  cl_int destroyCommandBuffer(mux_command_buffer_t command_buffer);

  /// @brief Create or get a cached semaphore.
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `context->getCommandQueueMutex()` when calling it.
  ///
  /// @return Returns the expected semaphore or `CL_OUT_OF_RESOURCES`.
  [[nodiscard]] cargo::expected<mux_shared_semaphore, cl_int> createSemaphore();

  /// @brief Drop ref count on  mux semaphore and delete if zero
  ///
  /// @note This member function is not thread-safe, callers **must** hold a
  /// lock on `context->getCommandQueueMutex()` when calling it.
  ///
  /// @param semaphore a mux semaphore.
  /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
  cl_int releaseSemaphore(mux_shared_semaphore semaphore);

  /// @brief Completion callback for user events.
  ///
  /// @param[in] user_event The user event that has completed.
  /// @param[in] event_command_exec_status The completion status of the event.
  /// @param[in] user_data User data to pass to the callback.
  static void userEventDispatch(cl_event user_event,
                                cl_int event_command_exec_status,
                                void *user_data);

  struct dispatch_state_t {
    /// @brief Add new wait events to this dispatch.
    ///
    /// @param event_wait_list List of events to wait for.
    ///
    /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
    [[nodiscard]] cl_int addWaitEvents(
        cargo::array_view<const cl_event> event_wait_list);

    /// @brief Add a new signal event to this dispatch.
    ///
    /// @param event Event to signal on completion.
    ///
    /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
    [[nodiscard]] cl_int addSignalEvent(cl_event event);

    /// @brief Register a new callback to invoke on dispatch completion.
    ///
    /// @param callback Callback to invoke on completion.
    ///
    /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
    [[nodiscard]] cl_int addCallback(std::function<void()> callback);

    /// @brief List of events this dispatch must wait for.
    cargo::small_vector<cl_event, 8> wait_events;
    /// @brief List of events this dispatch must signal on completion.
    cargo::small_vector<cl_event, 8> signal_events;

    /// @brief List of semaphores this dispatch must wait for.
    cargo::small_vector<mux_shared_semaphore, 8> wait_semaphores;
    /// @brief The semaphore which signals this dispatch is complete.
    mux_shared_semaphore signal_semaphore;

    /// @brief List of callbacks to invoke on completion.
    cargo::small_vector<std::function<void()>, 8> callbacks;

    /// @brief Flag specifying if the command buffer is associated with a
    /// _cl_command_buffer_khr object.
    bool is_user_command_buffer;

    /// @brief Flag specifying if it is the responsibility of the command queue
    /// to destroy  the command buffer. This is true for non-user command
    /// buffers and user command buffers which have been cloned.
    bool should_destroy_command_buffer;
  };

  /// @brief Ordered list of pending command buffers.
  cargo::small_vector<mux_command_buffer_t, 16> pending_command_buffers;
  /// @brief Mapping from command buffer to dispatch information.
  std::unordered_map<mux_command_buffer_t, dispatch_state_t> pending_dispatches;
  /// @brief Mapping from command buffer to fence.
  /// TODO: This is probably not the best way to do this. Fences can be reset,
  /// so we could create a pool of them and reuse them as they are signaled.
  std::unordered_map<mux_command_buffer_t, mux_fence_t> fences;

  /// @brief State required for tracking a running command buffer.
  struct running_state_t {
    /// @brief The command buffer which is currently running.
    mux_command_buffer_t command_buffer;
    /// @brief The list of semaphores this dispatch is waiting for.
    cargo::small_vector<mux_shared_semaphore, 8> wait_semaphores;
    /// @brief The semaphore which signals this dispatch is complete.
    mux_shared_semaphore signal_semaphore;
    /// @brief Flag specifying if the command buffer is associated with a
    /// _cl_command_buffer_khr object.
    bool is_user_command_buffer;
    /// @brief Flag specifying if it is the responsibility of the command queue
    /// to destroy  the command buffer. This is true for non-user command
    /// buffers and user command buffers which have been cloned.
    bool should_destroy_command_buffer;
  };

  /// @brief Double ended queue to track currently running command buffers.
  std::deque<running_state_t> running_command_buffers;

  /// @brief State requiring destruction on command buffer dispatch finishing.
  struct finish_state_t {
    /// @brief Add command buffer state for future cleanup.
    ///
    /// @param command_queue The command queue owning the command buffer.
    /// @param signal_events Events associated with the command buffer.
    /// @param callbacks Callbacks to destroy additional state.
    ///
    /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES`.
    cl_int addState(cl_command_queue command_queue,
                    cargo::small_vector<cl_event, 8> signal_events,
                    cargo::small_vector<std::function<void()>, 8> callbacks);

    /// @brief Handles clearing of a finished command buffers state.
    ///
    /// Takes a lock on the command queue mutex to remove this instance from
    /// the `finish_state` map _if_ the `locked` parameter is `false`.
    ///
    /// @param command_buffer The command buffer associated with the state.
    /// @param error The result of the command buffer dispatch.
    /// @param locked Flag to signify is the command queue mutex is locked.
    void clear(mux_command_buffer_t command_buffer, mux_result_t error,
               bool locked);

    /// @brief The command queue which owns the command buffer.
    cl_command_queue command_queue;
    /// @brief The list of events associated with the command buffer.
    cargo::small_vector<cl_event, 8> signal_events;
    /// @brief The list of destroy callbacks associated with the command buffer.
    cargo::small_vector<std::function<void()>, 8> callbacks;
  };

  /// @brief Storage for command buffer state to be destroyed on completion.
  ///
  /// Instances of `finish_state_t*` are passed as the `user_data` parameter of
  /// `muxDispatch` to be called when the dispatch is completed, once the
  /// state has been destroyed it is removed from this storage by the
  /// `destroyStateOf` member function. `std::unordered_map` guarantees that
  /// pointers and references to elements are not invalidated until they are
  /// removed from the data structure, we rely on this when passing a pointer
  /// to a `muxDispatch`'s callback `user_data` argument.
  std::unordered_map<mux_command_buffer_t, finish_state_t> finish_state;

  /// @brief A set of command buffers that are idle and ready to use.
  cargo::ring_buffer<mux_command_buffer_t, 16> cached_command_buffers;

  /// @brief List of completed signal semaphores which are still being waited
  /// on by running dispatches.
  cargo::small_vector<mux_shared_semaphore, 32> completed_signal_semaphores;

#ifdef OCL_EXTENSION_cl_khr_command_buffer
  /// @brief A map of mux_command_buffer_t to their associated
  /// _cl_command_buffer_khrs which have been enqueued to the command queue.
  std::unordered_map<mux_command_buffer_t, cl_command_buffer_khr>
      user_command_buffers;
#endif

  /// true if we are currently in flush() on this command queue
  /// This helps us avoid an infinite loop on flushing, as we can
  /// call flushes on other command queues if there is a cross
  /// queue event dependency.
  bool in_flush;
};

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Create an OpenCL command queue object.
///
/// @param context Context the command queue belongs to.
/// @param device Device the command queue will target.
/// @param properties Bitfield of properties the queue should enable such as
/// out of order execution or profiling, may be zero.
/// @param errcode_ret Return error code if not null.
///
/// @return Return the new command queue object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateCommandQueue.html
CL_API_ENTRY cl_command_queue CL_API_CALL
CreateCommandQueue(cl_context context, cl_device_id device,
                   cl_command_queue_properties properties, cl_int *errcode_ret);

/// @brief Increment the command queue reference count.
///
/// @param command_queue Command queue to increment the reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clRetainCommandQueue.html
CL_API_ENTRY cl_int CL_API_CALL
RetainCommandQueue(cl_command_queue command_queue);

/// @brief Decrement the command queue reference count.
///
/// @param command_queue Command queue to decrement the reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clReleaseCommandQueue.html
CL_API_ENTRY cl_int CL_API_CALL
ReleaseCommandQueue(cl_command_queue command_queue);

/// @brief Query the command queue for information.
///
/// @param command_queue Command queue to query for information.
/// @param param_name Type of information to query.
/// @param param_value_size Size in bytes of @p param_value storage.
/// @param param_value Pointer to value to store information, may be null.
/// @param param_value_size_ret Return size in bytes required for @p
/// param_value to store information if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetCommandQueueInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetCommandQueueInfo(
    cl_command_queue command_queue, cl_command_queue_info param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret);

/// @brief Enqueue a memory barrier to the queue.
///
/// @param[in] queue Command queue to enqueue the barrier on.
/// @param[in] num_events Number of event in the @a event_list.
/// @param[in] event_list List of events to wait on.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueBarrierWithWaitList.html
CL_API_ENTRY cl_int CL_API_CALL
EnqueueBarrierWithWaitList(cl_command_queue queue, cl_uint num_events,
                           const cl_event *event_list, cl_event *event);

/// @brief Enqueue a marker to wait for execution to complete.
///
/// @param[in] queue Command queue to enqueue marker on.
/// @param[in] num_events Number of events in `event_list`.
/// @param[in] event_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueMarkerWithWaitList.html
CL_API_ENTRY cl_int CL_API_CALL
EnqueueMarkerWithWaitList(cl_command_queue queue, cl_uint num_events,
                          const cl_event *event_list, cl_event *event);

/// @brief Enqueue an event wait list, deprecated in OpenCL 1.2.
///
/// @param[in] queue Command queue to enqueue wait on.
/// @param[in] num_events Number of events in the `event_list`.
/// @param[in] event_list List of event to wait for.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clEnqueueWaitForEvents.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueWaitForEvents(
    cl_command_queue queue, cl_uint num_events, const cl_event *event_list);

/// @brief Submit all previously enqueued commands for execution.
///
/// @param command_queue Command queue to submit commands on.
///
/// @return Return error code.
///
/// @see http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clFlush.html
CL_API_ENTRY cl_int CL_API_CALL Flush(cl_command_queue command_queue);

/// @brief Blocks until all previously enqueued commands are complete.
///
/// @param command_queue Command queue to submit commands on.
///
/// @return Return error code.
///
/// @see http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clFinish.html
CL_API_ENTRY cl_int CL_API_CALL Finish(cl_command_queue command_queue);

/// @brief Enqueue a barrier on the command queue, deprecated in OpenCL 1.2.
///
/// @param queue Command queue to enqueue the barrier on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clEnqueueBarrier.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueBarrier(cl_command_queue queue);

/// @brief Enqueue a marker on the command queue, deprecated in OpenCL 1.2.
///
/// @param queue Command queue to enqueue the marker on.
/// @param event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clEnqueueMarker.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueMarker(cl_command_queue queue,
                                              cl_event *event);

/// @}
}  // namespace cl

#endif  // CL_COMMAND_QUEUE_H_INCLUDED
