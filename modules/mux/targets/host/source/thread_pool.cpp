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

#include <host/thread_pool.h>

#include <algorithm>

namespace {

/// Number of threads in pool is total_cores - ca_free_hw_threads.
/// It's hard to pick a number that suits everything, it will depend
/// on a number of factors. If calling code has little overhead consider
/// reducing this to zero.
constexpr size_t ca_free_hw_threads = 0;

/// The code to do one iteration of the threadFunc loop.
void threadFuncBody(host::thread_pool_s *const me,
                    host::thread_pool_work_item_s item) {
  const tracer::TraceGuard<tracer::Impl> traceGuard(__func__);

  item.function(item.user_data, item.user_data2, item.user_data3, item.index);
  // technically our condition variable doesn't need this to be locked, but
  // in practice we could erroneously wait for work in wait() if we don't
  // hold this mutex before signalling the work item is complete.
  bool reached_zero{false};
  {
    const std::lock_guard<std::mutex> guard(me->wait_mutex);

    // Signal that we've completed this bit of work.  Count gets decremented
    // after signal gets set because if a program is waiting on a single
    // command-group to finish the global count does not matter, but if a user
    // is waiting on the entire queue to finish we need to ensure that we are
    // completely done with all command-groups (i.e. set item.signal) before
    // item.count reaches zero.
    // Signal is optional, it could be null.
    if (item.signal) {
      *(item.signal) = true;
    }
    *(item.count) -= 1u;
    reached_zero = (*item.count == 0);
  }

  if (reached_zero) {
    me->finished.notify_all();
  }

  // signal anything waiting that the work is complete
  me->done_work.notify_all();
}

/// The function for each cargo::thread to call.
void threadFunc(host::thread_pool_s *const me) {
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  me->registerPid();
#endif
  host::thread_pool_work_item_s item;
  while (me->getWork(&item)) {
    threadFuncBody(me, item);
  }
}
}  // namespace

namespace host {
thread_pool_s::thread_pool_s() : stayAlive(true) {
  const tracer::TraceGuard<tracer::Impl> traceGuard(__func__);

  auto clamp = [](size_t v, size_t a, size_t b) {
    const size_t start = std::min(a, b);
    const size_t end = std::max(a, b);

    return std::max(start, std::min(v, end));
  };

  const size_t hw_threads = cargo::thread::hardware_concurrency();
  const size_t desired_threads =
      clamp(hw_threads - ca_free_hw_threads, 2, hw_threads);
  const size_t max_threads = thread_pool_s::max_num_threads;
  size_t debug_threads = thread_pool_s::max_num_threads;

  // Register the value of the CA_HOST_NUM_THREADS environment variable.
  // If the programmer has provided an override to the number of threads that
  // the pool should use then note that we treat this as a cap, i.e. if the
  // programmer sets a high number it won't necessarily have an effect.
  const char *env = std::getenv("CA_HOST_NUM_THREADS");
  if (nullptr != env) {
    if (const int t = std::atoi(env)) {
      debug_threads = t;
    }
  }

  // Must be set before num_threads() is called.
  initialized_threads = std::min({desired_threads, max_threads, debug_threads});
  for (size_t i = 0, e = num_threads(); i < e; i++) {
    pool[i] = cargo::thread(threadFunc, this);
    pool[i].set_name("host:pool:" + std::to_string(i));
  }
}

thread_pool_s::~thread_pool_s() {
  const tracer::TraceGuard<tracer::Impl> traceGuard(__func__);

  {
    const std::lock_guard<std::mutex> guard(mutex);
    // kill the thread pool
    stayAlive = false;
  }
  // wake up all our threads
  new_work.notify_all();

  // wait for all our threads
  for (size_t i = 0, e = num_threads(); i < e; i++) {
    pool[i].join();
  }
}

bool thread_pool_s::getWork(thread_pool_work_item_s *const work) {
  if (!stayAlive) {
    return false;
  }

  std::unique_lock<std::mutex> guard(mutex);

  new_work.wait(guard, [&] {
    return (queue_read_index != queue_write_index) || !(stayAlive);
  });

  // This tracer is placed later so we get nice gaps in the graph when the
  // thread pool is just waiting.
  const tracer::TraceGuard<tracer::Impl> traceGuard(__func__);

  if (!stayAlive) {
    return false;
  }

  *work = queue[queue_read_index];

  queue_read_index = (queue_read_index + 1) % queue_max;

  return true;
}

bool thread_pool_s::tryGetWork(thread_pool_work_item_s *const work) {
  const tracer::TraceGuard<tracer::Impl> traceGuard(__func__);

  if (!stayAlive) {
    return false;
  }

  const std::unique_lock<std::mutex> guard(mutex);

  if (queue_read_index == queue_write_index) {
    return false;
  }

  *work = queue[queue_read_index];

  queue_read_index = (queue_read_index + 1) % queue_max;

  return true;
}

size_t thread_pool_s::num_threads() const { return this->initialized_threads; }

void thread_pool_s::enqueue(function_t function, void *user_data,
                            void *user_data2, void *user_data3, size_t index,
                            std::atomic<bool> *signal,
                            std::atomic<uint32_t> *count) {
  const tracer::TraceGuard<tracer::Impl> traceGuard(__func__);

  // Count gets incremented before signal gets set.
  *count += 1u;
  // The signal is optional and could be null.
  if (signal) {
    *signal = false;
  }

  {
    std::unique_lock<std::mutex> lock(mutex);

    unsigned next_write_index = (queue_write_index + 1) % queue_max;

    while (queue_read_index == next_write_index) {
      // We've entirely filled our work buffer! Need to wait until a space
      // opens so unlock the queue mutex, acquire the wait mutex, notify the
      // pool that some work needs doing and wait for an item to complete.
      {
        std::unique_lock<std::mutex> wait_lock(wait_mutex);
        lock.unlock();
        new_work.notify_one();
        done_work.wait(wait_lock);
        lock.lock();
      }

      next_write_index = (queue_write_index + 1) % queue_max;
    }

    queue[queue_write_index] = {function, user_data, user_data2, user_data3,
                                index,    signal,    count};

    queue_write_index = next_write_index;
  }

  new_work.notify_one();
}

void thread_pool_s::wait(std::atomic<bool> *signal) {
  const tracer::TraceGuard<tracer::Impl> traceGuard(__func__);

  // If the signal hasn't been triggered, lets jump in and help the thread pool
  // execute! We need to help out the main thread pool with our current thread
  // because of the way our host queue's execute nd-range commands - these
  // commands are sliced into sections and then re-enqueued on the thread pool.
  // If our wait here did not help in executing we could (and will) hit a
  // deadlock in the case of this being the only thread executing work.
  if (false == *signal) {
    host::thread_pool_work_item_s item;
    while ((false == *signal) && tryGetWork(&item)) {
      threadFuncBody(this, item);
    }

    // Now we check if the signal is done.
    if (false == *signal) {
      std::unique_lock<std::mutex> guard(wait_mutex);
      done_work.wait(guard, [signal] { return signal->load(); });
    }
  }
}

void thread_pool_s::wait(std::atomic<uint32_t> *count) {
  const tracer::TraceGuard<tracer::Impl> traceGuard(__func__);

  // If the counter hasn't been triggered, lets jump in and help the thread pool
  // execute! We need to help out the main thread pool with our current thread
  // because of the way our host queue's execute nd-range commands - these
  // commands are sliced into sections and then re-enqueued on the thread pool.
  // If our wait here did not help in executing we could (and will) hit a
  // deadlock in the case of this being the only thread executing work.
  if (*count != 0) {
    host::thread_pool_work_item_s item;
    while ((*count != 0) && tryGetWork(&item)) {
      threadFuncBody(this, item);
    }

    // Now we check if the count has reached zero.
    if (*count != 0) {
      std::unique_lock<std::mutex> guard(wait_mutex);
      finished.wait(guard, [count] { return *count == 0; });
    }
  }
}
}  // namespace host
