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

#ifndef RISCV_LIB_BAKERY_H
#define RISCV_LIB_BAKERY_H

#include <cstdint>

/// @brief gets the data which represents the compiled runtime library for 64
/// bit RISC-V
/// @return Return a pointer to the data
const uint8_t *get_rtlib64_data();

/// @brief gets size of the 64 bit runtime library data
/// @return Return the size in bytes of the runtime library data
size_t get_rtlib64_size();

/// @brief gets the data which represents the compiled runtime library for 32
/// bit RISC-V
/// @return Return a pointer to the data
const uint8_t *get_rtlib32_data();

/// @brief gets size of the 32 bit runtime library data
/// @return Return the size in bytes of the runtime library data
size_t get_rtlib32_size();

#endif
