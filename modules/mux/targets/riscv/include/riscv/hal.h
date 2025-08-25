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
/// riscv's hal interface.

#ifndef RISCV_HAL_H_INCLUDED
#define RISCV_HAL_H_INCLUDED

#include <hal.h>
#include <hal_library.h>

namespace riscv {

/// @brief Load the hal library if needed and return an hal_t instance.
///
///  returns nullptr on failure.
hal::hal_t *hal_get();

/// @brief Unload the hal library.
void hal_unload();

}  // namespace riscv

#endif  // RISCV_HAL_H_INCLUDED
