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
/// @brief HAL base implementation of the mux_kernel_s object.

#ifndef MUX_HAL_KERNEL_H_INCLUDED
#define MUX_HAL_KERNEL_H_INCLUDED

#include "cargo/array_view.h"
#include "cargo/expected.h"
#include "cargo/small_vector.h"
#include "cargo/string_view.h"
#include "mux/hal/executable.h"
#include "mux/mux.h"
#include "mux/utils/dynamic_array.h"

namespace mux {
namespace hal {
struct device;
struct executable;

struct kernel_variant_s {
  /// @brief The (compiler-generated) name of this kernel variant.
  cargo::string_view variant_name;
  /// @brief The factor of the minimum work-group size which this kernel must
  /// execute.
  uint32_t min_work_width = 0;
  /// @brief The factor of the work-group size at which this kernel performs
  /// best.
  uint32_t pref_work_width = 0;
  /// @brief The size of the sub-group this kernel variant supports.
  ///
  /// Note that the last sub-group in a work-group may be smaller than this
  /// value.
  /// * If one, denotes a trivial sub-group.
  /// * If zero, denotes a 'degenerate' sub-group (i.e., the size of the
  /// work-group at enqueue time).
  uint32_t sub_group_size = 0;
};

template <class VariantData>
struct kernel : mux_kernel_s {
  /// @brief Construct a kernel object.
  ///
  /// @param[in] device Mux device.
  /// @param[in] name Name of the requested kernel.
  /// @param[in] object_code View into the ELF object code.
  kernel(mux::hal::device *device, cargo::string_view name,
         cargo::array_view<uint8_t> object_code, mux::allocator allocator,
         cargo::small_vector<VariantData, 4> &&variant_data)
      : name(cargo::as<std::string>(name)),
        object_code(object_code),
        allocator(allocator),
        variant_data(std::move(variant_data)) {
    this->device = device;
  }

  template <class Kernel>
  static cargo::expected<Kernel *, mux_result_t> create(
      mux::hal::device *device, mux::hal::executable *executable,
      cargo::string_view name,
      cargo::small_vector<VariantData, 4> &&variant_data,
      mux::allocator allocator) {
    static_assert(std::is_base_of<mux::hal::kernel<VariantData>, Kernel>::value,
                  "template type Kernel must be derived from mux::hal::kernel");
    auto kernel =
        allocator.create<Kernel>(device, name, executable->object_code,
                                 allocator, std::move(variant_data));
    if (nullptr == kernel) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
    return kernel;
  }

  template <class Kernel>
  static void destroy(mux::hal::device *device, Kernel *kernel,
                      mux::allocator allocator) {
    static_assert(std::is_base_of<mux::hal::kernel<VariantData>, Kernel>::value,
                  "template type Kernel must be derived from mux::hal::kernel");
    (void)device;
    allocator.destroy(kernel);
  }

  /// Exactly how kernel variants are selected is an implementation detail.
  mux_result_t getKernelVariantForWGSize(size_t local_size_x,
                                         size_t local_size_y,
                                         size_t local_size_z,
                                         VariantData *out_variant_data) {
    (void)local_size_x;
    (void)local_size_y;
    (void)local_size_z;
    (void)out_variant_data;
    return mux_error_feature_unsupported;
  }

  /// HAL devices are not required to support sub-groups and they are device
  /// dependent, as such mux::hal::kernel does not support sub-groups.
  ///
  /// @see muxQuerySubGroupSizeForLocalSize
  mux_result_t getSubGroupSizeForLocalSize(size_t local_size_x,
                                           size_t local_size_y,
                                           size_t local_size_z,
                                           size_t *out_sub_group_size) {
    (void)local_size_x;
    (void)local_size_y;
    (void)local_size_z;
    (void)out_sub_group_size;
    return mux_error_feature_unsupported;
  }

  /// HAL devices are not required to support sub-groups and they are device
  /// dependent, as such mux::hal::kernel does not support sub-groups.
  ///
  /// @see muxQueryWFVInfoForLocalSize
  mux_result_t getWFVInfoForLocalSize(size_t local_size_x, size_t local_size_y,
                                      size_t local_size_z,
                                      mux_wfv_status_e *out_wfv_status,
                                      size_t *out_work_width_x,
                                      size_t *out_work_width_y,
                                      size_t *out_work_width_z) {
    (void)local_size_x;
    (void)local_size_y;
    (void)local_size_z;
    (void)out_wfv_status;
    (void)out_work_width_x;
    (void)out_work_width_y;
    (void)out_work_width_z;
    return mux_error_feature_unsupported;
  }

  /// HAL devices are not required to support sub-groups and they are device
  /// dependent, as such mux::hal::kernel does not support sub-groups.
  ///
  /// @see muxQueryLocalSizeForSubGroupCount
  mux_result_t getLocalSizeForSubGroupCount(size_t sub_group_count,
                                            size_t *out_local_size_x,
                                            size_t *out_local_size_y,
                                            size_t *out_local_size_z) {
    (void)sub_group_count;
    (void)out_local_size_x;
    (void)out_local_size_y;
    (void)out_local_size_z;
    return mux_error_feature_unsupported;
  }

  /// @brief Name of the kernel.
  ///
  /// This is one of the kernels available in the binary. Note this must be
  /// null terminated
  std::string name;

  /// @brief elf file containing kernel as binary code
  cargo::array_view<uint8_t> object_code;

  mux::allocator allocator;

  /// @brief The list of variants of this kernel, generated by the compiler.
  cargo::small_vector<VariantData, 4> variant_data;
};
}  // namespace hal
}  // namespace mux

#endif  // MUX_HAL_KERNEL_H_INCLUDED
