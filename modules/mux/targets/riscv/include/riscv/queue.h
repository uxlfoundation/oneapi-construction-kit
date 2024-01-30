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
/// Riscv's queue interface.

#ifndef RISCV_QUEUE_H_INCLUDED
#define RISCV_QUEUE_H_INCLUDED

#include <atomic>
#include <condition_variable>

#include "cargo/array_view.h"
#include "cargo/mutex.h"
#include "cargo/thread.h"
#include "mux/mux.h"
#include "mux/utils/small_vector.h"
#include "riscv/fence.h"
#include "riscv/semaphore.h"

namespace riscv {
/// @addtogroup riscv
/// @{

struct command_buffer_s;

struct dispatch_s {
  riscv::command_buffer_s *command_buffer = nullptr;
  riscv::fence_s *fence = nullptr;
  mux::small_vector<riscv::semaphore_s *, 8> wait_semaphores;
  mux::small_vector<riscv::semaphore_s *, 8> signal_semaphores;
  void (*user_function)(mux_command_buffer_t command_buffer, mux_result_t error,
                        void *const user_data) = nullptr;
  void *user_data = nullptr;

  void signal(mux_result_t result);

  void terminate();

  void notify_user(mux_result_t result);

  bool is_waiting() const;

  bool is_terminated() const;
};

struct queue_s final : public mux_queue_s {
  queue_s(mux::allocator allocator, mux_device_t device)
      : pending{allocator}, running{false}, terminate{false} {
    this->device = device;
    cargo::lock_guard<cargo::mutex> lock(mutex);
    thread = cargo::thread{[this]() { run(); }};
    thread.set_name("riscv:queue");
  }

  /// @brief Destructor.
  ~queue_s() {
    {
      cargo::lock_guard<cargo::mutex> lock(mutex);
      terminate = true;
      condition_variable.notify_all();
    }
    thread.join();
  }

  [[nodiscard]] mux_result_t dispatch(
      riscv::command_buffer_s *command_buffer,
      cargo::array_view<riscv::semaphore_s *> wait_semaphores,
      cargo::array_view<riscv::semaphore_s *> signal_semaphores,
      riscv::fence_s *fence,
      void (*user_function)(mux_command_buffer_t command_buffer,
                            mux_result_t error, void *const user_data),
      void *user_data);

  void run();

  mux::small_vector<riscv::dispatch_s, 32> pending CARGO_TS_GUARDED_BY(mutex);

  cargo::thread thread;
  cargo::mutex mutex;
  std::condition_variable_any condition_variable;

  bool running CARGO_TS_GUARDED_BY(mutex);
  bool terminate CARGO_TS_GUARDED_BY(mutex);
};

/// @}
}  // namespace riscv

#endif  // RISCV_QUEUE_H_INCLUDED
