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
/// riscv's kernel interface.

#ifndef RISCV_KERNEL_H_INCLUDED
#define RISCV_KERNEL_H_INCLUDED

#include "mux/hal/device.h"
#include "mux/hal/kernel.h"

namespace riscv {
struct device_s;
struct executable_s;

struct kernel_s final : mux::hal::kernel<mux::hal::kernel_variant_s> {
  /// @brief Construct a kernel object.
  ///
  /// @param[in] device Mux device.
  /// @param[in] name Name of the requested kernel.
  /// @param[in] object_code View into the ELF object code.
  /// @param[in] variant_data The array of variants of this kernel.
  kernel_s(mux::hal::device *device, cargo::string_view name,
           cargo::array_view<uint8_t> object_code, mux::allocator allocator,
           cargo::small_vector<mux::hal::kernel_variant_s, 4> &&variant_data);

  static cargo::expected<riscv::kernel_s *, mux_result_t>
  create(riscv::device_s *device, riscv::executable_s *executable,
         cargo::string_view name, mux::allocator allocator);

  mux_result_t getSubGroupSizeForLocalSize(size_t local_size_x,
                                           size_t local_size_y,
                                           size_t local_size_z,
                                           size_t *out_sub_group_size);

  mux_result_t getLocalSizeForSubGroupCount(size_t sub_group_count,
                                            size_t *out_local_size_x,
                                            size_t *out_local_size_y,
                                            size_t *out_local_size_z);

  mux_result_t
  getKernelVariantForWGSize(size_t local_size_x, size_t local_size_y,
                            size_t local_size_z,
                            mux::hal::kernel_variant_s *out_variant_data);
};

} // namespace riscv

#endif // RISCV_KERNEL_H_INCLUDED
