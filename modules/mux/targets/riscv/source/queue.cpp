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

#include <cargo/optional.h>
#include <mux/config.h>
#include <mux/mux.h>
#include <riscv/buffer.h>
#include <riscv/command_buffer.h>
#include <riscv/device.h>
#include <riscv/kernel.h>
#include <riscv/query_pool.h>
#include <riscv/queue.h>
#include <riscv/riscv.h>
#include <riscv/semaphore.h>
#include <utils/system.h>

#include <cassert>
#include <cstring>
#include <memory>
#include <new>
#include <vector>

namespace riscv {
void dispatch_s::signal(mux_result_t result)
    CARGO_TS_REQUIRES(command_buffer->mutex) {
  for (auto semaphore : signal_semaphores) {
    semaphore->signal();
  }
  // Notify the optional fence.
  if (fence) {
    fence->signal(result);
  }
}

void dispatch_s::terminate() {
  for (auto semaphore : signal_semaphores) {
    semaphore->terminate();
  }
}

void dispatch_s::notify_user(mux_result_t result) {
  if (user_function) {
    user_function(command_buffer, result, user_data);
  }
}

bool dispatch_s::is_waiting() const {
  return std::any_of(
      wait_semaphores.begin(), wait_semaphores.end(),
      [](riscv::semaphore_s *semaphore) { return !semaphore->is_signalled(); });
}

bool dispatch_s::is_terminated() const {
  return std::any_of(
      wait_semaphores.begin(), wait_semaphores.end(),
      [](riscv::semaphore_s *semaphore) { return semaphore->is_terminated(); });
}

[[nodiscard]] mux_result_t queue_s::dispatch(
    riscv::command_buffer_s *command_buffer,
    cargo::array_view<riscv::semaphore_s *> wait_semaphores,
    cargo::array_view<riscv::semaphore_s *> signal_semaphores,
    riscv::fence_s *fence,
    void (*user_function)(mux_command_buffer_t command_buffer,
                          mux_result_t error, void *const user_data),
    void *user_data) {
  cargo::lock_guard<cargo::mutex> command_buffer_lock{command_buffer->mutex};
  cargo::lock_guard<cargo::mutex> queue_lock{mutex};
  // Reset optional fence state.
  if (fence) {
    fence->reset();
  }
  // Create storage for our lists of samaphores.
  mux::small_vector<riscv::semaphore_s *, 8> wait_semaphores_storage{
      pending.get_allocator()};
  if (wait_semaphores_storage.assign(wait_semaphores.begin(),
                                     wait_semaphores.end())) {
    return mux_error_out_of_memory;
  }
  mux::small_vector<riscv::semaphore_s *, 8> signal_semaphores_storage{
      pending.get_allocator()};
  if (signal_semaphores_storage.assign(signal_semaphores.begin(),
                                       signal_semaphores.end())) {
    return mux_error_out_of_memory;
  }
  // Add the dispatch to the list of work to do.
  if (pending.push_back(
          {command_buffer, fence, std::move(wait_semaphores_storage),
           std::move(signal_semaphores_storage), user_function, user_data})) {
    return mux_error_out_of_memory;
  }
  // Notify the queue there is work to do.
  condition_variable.notify_all();
  return mux_success;
}

void queue_s::run() {
  for (;;) {
    cargo::optional<riscv::dispatch_s> dispatch = cargo::nullopt;

    {
      cargo::unique_lock<cargo::mutex> lock{mutex};
      // Wait work to be dispatched, or for termination signal.
      condition_variable.wait(lock, [this]() CARGO_TS_REQUIRES(mutex) {
        return pending.size() != 0 || terminate;
      });
      if (terminate) {
        break;
      }
      // Find the first dispatch which has no unsignalled wait semaphores.
      auto found = std::find_if(pending.begin(), pending.end(),
                                [&](const riscv::dispatch_s &dispatch) {
                                  return !dispatch.is_waiting();
                                });
      if (found != pending.end()) {
        // A dispatch that's not waiting was found, removing if from pending.
        dispatch = std::move(*found);
        pending.erase(found);
        running = true;
      }
    }

    if (!dispatch) {
      continue;
    }

    mux_result_t result = mux_error_failure;

    {
      cargo::lock_guard<cargo::mutex> lock{dispatch->command_buffer->mutex};
      // Execute the commands in the command buffer.
      if (!dispatch->is_terminated()) {
        result = dispatch->command_buffer->execute(this);
      }
    }

    // Notify the user via the dispatch callback. The queue and command-buffer
    // locks must not be held here because we allow muxDispatch() to be called
    // in this callback which also locks both.
    dispatch->notify_user(result);

    {
      cargo::lock_guard<cargo::mutex> command_buffer_lock{
          dispatch->command_buffer->mutex};
      cargo::lock_guard<cargo::mutex> queue_lock{mutex};
      if (result) {
        // There was an error, propogate termination flags.
        dispatch->terminate();
      }
      // Signal the semaphores that the command buffer is finished.
      dispatch->signal(result);
    }

    // Notify the waiters on the queue mutex. This is done without holding the
    // command buffer mutex, to avoid the following sequence of events:
    // 1) The queue is empty after dequeuing `dispatch`.
    // 2) `running` is set to false and other threads are notified that the
    // queue is empty.
    // 3) The queue thread releases the queue mutex. It is pre-empted by the OS,
    // still holding the command buffer lock.
    // 4) `muxWaitAll` returns on another thread. The caller deletes command
    // buffers, including the one still referenced by `dispatch`.
    // 5) The queue thread is resumed by the OS and tries to unlock the command
    // buffer mutex. The mutex has already been deleted, resulting in a crash.
    {
      cargo::lock_guard<cargo::mutex> queue_lock{mutex};
      running = false;
      condition_variable.notify_all();
    }
  }
}
}  // namespace riscv

mux_result_t riscvGetQueue(mux_device_t device, mux_queue_type_e, uint32_t,
                           mux_queue_t *out_queue) {
  auto riscvDevice = static_cast<riscv::device_s *>(device);

  *out_queue = &(riscvDevice->queue);

  return mux_success;
}

mux_result_t riscvDispatch(
    mux_queue_t queue, mux_command_buffer_t command_buffer, mux_fence_t fence,
    mux_semaphore_t *wait_semaphores, uint32_t wait_semaphores_length,
    mux_semaphore_t *signal_semaphores, uint32_t signal_semaphores_length,
    void (*user_function)(mux_command_buffer_t command_buffer,
                          mux_result_t error, void *const user_data),
    void *user_data) {
  return static_cast<riscv::queue_s *>(queue)->dispatch(
      static_cast<riscv::command_buffer_s *>(command_buffer),
      {reinterpret_cast<riscv::semaphore_s **>(wait_semaphores),
       wait_semaphores_length},
      {reinterpret_cast<riscv::semaphore_s **>(signal_semaphores),
       signal_semaphores_length},
      static_cast<riscv::fence_s *>(fence), user_function, user_data);
}

mux_result_t riscvTryWait(mux_queue_t queue, uint64_t timeout,
                          mux_fence_t fence) {
  return static_cast<riscv::fence_s *>(fence)->tryWait(timeout);
}

mux_result_t riscvWaitAll(mux_queue_t mux_queue) {
  auto queue = static_cast<riscv::queue_s *>(mux_queue);
  // Wait for all work to have left the queue.
  cargo::unique_lock<cargo::mutex> lock(queue->mutex);
  queue->condition_variable.wait(
      lock, [queue]() CARGO_TS_REQUIRES(queue->mutex) {
        return queue->pending.size() == 0 && !queue->running;
      });
  return mux_success;
}
