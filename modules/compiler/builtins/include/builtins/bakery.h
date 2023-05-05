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
///
/// @brief Binary bakery.

#ifndef BAKERY_BAKERY_H_INCLUDED
#define BAKERY_BAKERY_H_INCLUDED

#include <cstddef>
#include <cstdint>

#include "cargo/array_view.h"

namespace builtins {
/// @addtogroup builtins
/// @{

namespace file {
/// @brief Embedded file capabilities
enum capabilities : uint32_t {
  /// @brief Default (minimal) capabilities (64-bit).
  CAPS_DEFAULT = 0x0,
  /// @brief 32-bit file (default is 64-bit).
  CAPS_32BIT = 0x1,
  /// @brief File with floating point double types.
  CAPS_FP64 = 0x2,
  /// @brief File with floating point half types.
  CAPS_FP16 = 0x4
};

using capabilities_bitfield = uint32_t;
}  // namespace file

/// @brief Get the builtins header source.
///
/// @return Reference to the builtins header source.
cargo::array_view<const uint8_t> get_api_src_file();

/// @brief Get the builtins header source for OpenCL 3.0.
///
/// @return Reference to the builtins header source for OpenCL 3.0.
cargo::array_view<const uint8_t> get_api_30_src_file();

/// @brief Get the force-include header for a core device
///
/// @param[in] device_name Core device's device_name
///
/// @return Reference to the header, if available, or an empty file otherwise
cargo::array_view<const uint8_t> get_api_force_file_device(
    const char *const device_name);

/// @brief Get a builtins precompiled header based on the required capabilities.
///
/// @param[in] caps A capabilities_bitfield of the required capabilities.
///
/// @return Reference to the precompiled header.
cargo::array_view<const uint8_t> get_pch_file(file::capabilities_bitfield caps);

/// @brief Get a builtins bitcode file based on the required capabilities.
///
/// @param[in] caps A capabilities_bitfield of the required capabilities.
///
/// @return Reference to the bitcode file.
cargo::array_view<const uint8_t> get_bc_file(file::capabilities_bitfield caps);

/// @}
}  // namespace builtins

#endif  // BAKERY_BAKERY_H_INCLUDED
