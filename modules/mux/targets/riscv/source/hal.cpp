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
#include <hal.h>
#include <hal_library.h>
#include <riscv/hal.h>

namespace riscv {

/// @brief Current version of the HAL API. The version number needs to be
/// bumped any time the interface is changed.
static const uint32_t expected_hal_version = 6;

// hal instances
static hal::hal_library_t hal_library;
static hal::hal_t *hal_instance;

hal::hal_t *hal_get() {
  if (hal_instance) {
    // hal has already been loaded so just return it
    return hal_instance;
  }
  static_assert(expected_hal_version == hal::hal_t::api_version,
                "Expected HAL API version for Mux target does not match hal.h");
  hal_instance =
      hal::load_hal(CA_HAL_DEFAULT_DEVICE, expected_hal_version, hal_library);
  return hal_instance;
}

void hal_unload() {
  // discard the hal_t instance
  hal_instance = nullptr;
  // unload the hal library
  hal::unload_hal(hal_library);
  hal_library = nullptr;
}

}  // namespace riscv
