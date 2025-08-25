// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/// @file
/// Host's semaphore interface.

#ifndef HOST_SEMAPHORE_H_INCLUDED
#define HOST_SEMAPHORE_H_INCLUDED

#include <host/host.h>
#include <mux/mux.h>
#include <mux/utils/small_vector.h>

#include <mutex>

namespace host {
/// @addtogroup host
/// @{

struct command_buffer_s;
struct queue_s;

struct semaphore_s final : public mux_semaphore_s {
  explicit semaphore_s(mux_device_t device,
                       mux_allocator_info_t allocator_info);

  ~semaphore_s();

  void signal(bool terminate = false);

  mux_result_t addWait(mux_command_buffer_t group);

  void reset();

 private:
  bool signalled;
  bool failed;
  mux::small_vector<mux_command_buffer_t, 8> waitingGroups;
};

/// @}
}  // namespace host

#endif  // HOST_SEMAPHORE_H_INCLUDED
