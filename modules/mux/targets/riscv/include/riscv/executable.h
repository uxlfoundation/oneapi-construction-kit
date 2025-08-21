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
/// riscv's executable interface.

#ifndef RISCV_EXECUTABLE_H_INCLUDED
#define RISCV_EXECUTABLE_H_INCLUDED

#include <metadata/handler/vectorize_info_metadata.h>
#include <metadata/metadata.h>

#include "mux/hal/executable.h"
#include "mux/utils/small_vector.h"

namespace riscv {
struct device_s;

struct executable_s final : mux::hal::executable {
  /// @brief Create an executable from a pre-compiled binary.
  ///
  /// @param[in] device mux device.
  /// @param[in] object_code Contents of ELF file to take ownership of.
  executable_s(mux::hal::device *device,
               mux::dynamic_array<uint8_t> &&object_code);

  static cargo::expected<riscv::executable_s *, mux_result_t>
  create(riscv::device_s *device, const void *binary, uint64_t binary_length,
         mux::allocator allocator);

  static void destroy(riscv::device_s *device, riscv::executable_s *executable,
                      mux::allocator allocator);

  /// @brief per kernel information such as names and vectorization factor
  cargo::small_vector<handler::VectorizeInfoMetadata, 4> kernel_info;
};

/// @}
} // namespace riscv

#endif // RISCV_EXECUTABLE_H_INCLUDED
