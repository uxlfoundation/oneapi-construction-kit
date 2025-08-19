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
///
/// @brief Device Hardware Abstraction Layer common routines.

#ifndef RISCV_HAL_COMMON_H_INCLUDED
#define RISCV_HAL_COMMON_H_INCLUDED

#include <assert.h>
#include <hal.h>
#include <hal_riscv.h>
#include <hal_types.h>

namespace riscv {
/// @addtogroup riscv
/// @{

bool update_info_from_riscv_isa_description(
    const char *str, ::hal::hal_device_info_t &info,
    hal_device_info_riscv_t &riscv_info);
}  // namespace riscv

#endif
