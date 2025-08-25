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

#include "mux/hal/buffer.h"

namespace mux {
namespace hal {
buffer::buffer(mux_memory_requirements_s memory_requirements)
    : targetPtr(::hal::hal_nullptr) {
  this->memory_requirements = memory_requirements;
}

mux_result_t buffer::bind(mux_device_t device, mux::hal::memory *memory,
                          uint64_t offset) {
  (void)device;
  targetPtr = memory->targetPtr + offset;
  return mux_success;
}
}  // namespace hal
}  // namespace mux
