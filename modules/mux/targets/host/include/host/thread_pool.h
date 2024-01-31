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
/// Host's thread pool interface.

#ifndef HOST_THREAD_POOL_H_INCLUDED
#define HOST_THREAD_POOL_H_INCLUDED

#include <array>
#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <new>

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
#include <papi.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include "cargo/thread.h"
#include "tracer/tracer.h"

namespace host {
/// @addtogroup host
/// @{

/// The signature of our thread pool functions
typedef void (*function_t)(void *const, void *const, void *const, size_t);

struct thread_pool_work_item_s final {
  function_t function;
  void *user_data;
  void *user_data2;
  void *user_data3;
  size_t index;
  std::atomic<bool> *signal;
  std::atomic<uint32_t> *count;
};

struct thread_pool_s final {
  explicit thread_pool_s();

  ~thread_pool_s();

  /// @brief Blocking function get work to execute.
  /// @param[out] work The work item to execute.
  /// @return True if there was work to execute, false otherwise.
  bool getWork(thread_pool_work_item_s *const work);

  /// @brief Non-blocking function get work to execute.
  /// @param[out] work The work item to execute.
  /// @return True if there was work to execute, false otherwise.
  bool tryGetWork(thread_pool_work_item_s *const work);

  /// The number of threads supported in the thread pool.
  size_t num_threads() const;

  /// @brief Enqueue some work on the thread pool.
  /// @param[in] function The function to run in the thread pool.
  /// @param[in] user_data User data to pass to the function.
  /// @param[in] user_data2 A second user data to pass to the function.
  /// @param[in] user_data3 A third user data to pass to the function.
  /// @param[in] index An index that is passed to the function.
  /// @param[in,out] signal A bool that is set to true when the enqueued
  /// function has completed.
  /// @param[in,out] count A number that is incremented immediately, and
  /// decremented when the enqueued function has completed.
  void enqueue(function_t function, void *user_data, void *user_data2,
               void *user_data3, size_t index, std::atomic<bool> *signal,
               std::atomic<uint32_t> *count);

  /// @brief Enqueue a range worth of work on the thread pool.
  ///
  /// This has the advantage of holding onto `mutex` for the whole duration of
  /// the loop, meaning we never have to wait to re-aquire it as you would using
  /// `enqueue` in a loop.
  ///
  /// @param[in] function The function to run in the thread pool.
  /// @param[in] user_data User data to pass to the function.
  /// @param[in] user_data2 A second user data to pass to the function.
  /// @param[in] user_data3 A third user data to pass to the function.
  /// @param[in,out] signals A list of bools that will be signalled when each
  /// slice of the enqueue range has completed.
  /// @param[in,out] count A number that is incremented immediately, and
  /// decremented when the enqueued function has completed.
  /// @param[in] slices The number of pieces that the work is to be divided into
  /// when it is enqueued on the thread pool.
  ///
  /// @tparam N Length of `signals`.
  template <size_t N>
  void enqueue_range(function_t function, void *user_data, void *user_data2,
                     std::array<std::atomic<bool>, N> &signals,
                     std::atomic<uint32_t> *count, size_t slices) {
    const tracer::TraceGuard<tracer::Impl> traceGuard(__func__);

    {
      std::unique_lock<std::mutex> lock(mutex);

      for (size_t index = 0; index < slices; index++) {
        // Count gets incremented before signal gets set.
        *count += 1u;
        signals[index] = false;

        unsigned next_write_index = (queue_write_index + 1) % queue_max;

        while (queue_read_index == next_write_index) {
          // We've entirely filled our work buffer! Need to wait until a space
          // opens so unlock the queue mutex, acquire the wait mutex, notify the
          // pool that some work needs doing and wait for an item to complete.
          {
            std::unique_lock<std::mutex> wait_guard(wait_mutex);
            lock.unlock();
            new_work.notify_one();
            done_work.wait(wait_guard);
            lock.lock();
          }

          next_write_index = (queue_write_index + 1) % queue_max;
        }

        queue[queue_write_index] = {
            function, user_data,         user_data2, nullptr,
            index,    &(signals[index]), count,
        };

        queue_write_index = next_write_index;
      }
    }

    new_work.notify_all();
  }

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  /// @brief Register the calling thread's system thread ID in `thread_ids`.
  void registerPid() {
    std::lock_guard<std::mutex> lock(thread_ids_mutex);
    thread_ids[std::this_thread::get_id()] = syscall(SYS_gettid);
  }

  /// @brief Mutex for controlling access to `thread_pids`.
  std::mutex thread_ids_mutex;
  /// @brief Mapping of cargo::thread::id to the analagous system thread pid_t.
  ///
  /// PAPI's thread related APIs work with system thread IDs, so we need to
  /// store them during initialization, and to be able to look them up later.
  std::map<cargo::thread::id, pid_t> thread_ids;
#endif

  /// @brief Wait for a signal to complete.
  /// @param[in,out] signal A signal that previously was passed to a call to
  /// enqueue, wait() will wait on the thread that is executing the work to
  /// complete.
  void wait(std::atomic<bool> *signal);

  /// @brief Wait for a batch of work to complete.
  /// @param[in,out] count A counter that previously was passed to a call to
  /// enqueue, wait() will wait for the counter to reach zero.
  void wait(std::atomic<uint32_t> *count);

  /// The maximum number of threads our thread pool supports. Useful for
  /// allocating memory (you know the max size of allocations required).
  static const size_t max_num_threads = 32;

  /// The maximum number of work that can be enqueued.
  static const size_t queue_max = 4096;

  /// The number of threads actually initialized in the thread pool.  General
  /// the lower of the number of cores or max_num_threads, but could be lower in
  /// the presence of debug settings.
  size_t initialized_threads;

  /// The pool of threads to use for execution.
  std::array<cargo::thread, max_num_threads> pool;

  /// The buffer to hold the queue of work.
  std::array<thread_pool_work_item_s, queue_max> queue;

  /// The read index into the queue.
  unsigned queue_read_index = 0;

  /// The write index from the queue.
  unsigned queue_write_index = 0;

  /// A mutex to use when accessing the thread pool.
  std::mutex mutex;

  /// A mutex to use when decrementing the work counter.
  std::mutex wait_mutex;

  /// A condition to signal when new work has been added.
  std::condition_variable new_work;

  /// A condition to signal when work has been done.
  std::condition_variable done_work;

  /// A condition to signal when the count reaches zero.
  std::condition_variable finished;

  /// A variable to query whether the thread pool is still alive or not.
  std::atomic<bool> stayAlive;
};

/// @}
}  // namespace host

#endif  // HOST_THREAD_POOL_H_INCLUDED
