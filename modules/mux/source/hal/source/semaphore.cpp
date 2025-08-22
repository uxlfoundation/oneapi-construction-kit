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

#include "mux/hal/semaphore.h"

#include "mux/utils/allocator.h"

namespace mux {
namespace hal {
semaphore::semaphore(mux_device_t device) : status{0} { this->device = device; }

void semaphore::signal() { status |= states::SIGNAL; }
bool semaphore::is_signalled() {
  return (status & states::SIGNAL) == states::SIGNAL;
}

void semaphore::reset() { status = 0; }

void semaphore::terminate() { status |= states::TERMINATE; }
bool semaphore::is_terminated() {
  return (status & states::TERMINATE) == states::TERMINATE;
}
} // namespace hal
} // namespace mux
