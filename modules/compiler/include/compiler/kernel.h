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
/// @brief Compiler kernel API.

#ifndef COMPILER_KERNEL_H_INCLUDED
#define COMPILER_KERNEL_H_INCLUDED

#include <cargo/dynamic_array.h>
#include <cargo/expected.h>
#include <compiler/result.h>
#include <mux/mux.hpp>

#include <string>

namespace compiler {
/// @addtogroup compiler
/// @{

/// @brief A class that represents a kernel, contained within a `Module`, where
/// compilation can be deferred to enqueue time.
class Kernel {
public:
  /// @brief Default constructor.
  ///
  /// @param name Kernel name.
  /// @param preferred_local_size_x The preferred local size in the x dimension
  /// for this kernel.
  /// @param preferred_local_size_y The preferred local size in the y dimension
  /// for this kernel.
  /// @param preferred_local_size_z The preferred local size in the z dimension
  /// for this kernel.
  /// @param local_memory_size The amount of local memory used by this kernel.
  Kernel(const std::string &name, size_t preferred_local_size_x,
         size_t preferred_local_size_y, size_t preferred_local_size_z,
         size_t local_memory_size)
      : name(name), preferred_local_size_x(preferred_local_size_x),
        preferred_local_size_y(preferred_local_size_y),
        preferred_local_size_z(preferred_local_size_z),
        local_memory_size(local_memory_size) {}

  /// @brief Virtual destructor.
  virtual ~Kernel() = default;

  /// @brief This function causes the compiler to pre-cache a specific local
  /// size configuration requested by createSpecializedKernel.
  ///
  /// @param local_size_x Local size in the x dimension.
  /// @param local_size_y Local size in the y dimension.
  /// @param local_size_z Local size in the z dimension.
  ///
  /// @return Returns a status code.
  /// @retval `Result::SUCCESS` when precaching the local size was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_VALUE` if the requested local size is invalid.
  virtual Result precacheLocalSize(size_t local_size_x, size_t local_size_y,
                                   size_t local_size_z) = 0;

  /// @brief Returns the dynamic work width for a given local size.
  ///
  /// @param local_size_x Local size in the x dimension.
  /// @param local_size_y Local size in the y dimension.
  /// @param local_size_z Local size in the z dimension.
  ///
  /// @return Returns the dynamic work width for the given local size, or a
  /// status code if it was unsuccessful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_VALUE` if the requested local size is invalid.
  virtual cargo::expected<uint32_t, Result>
  getDynamicWorkWidth(size_t local_size_x, size_t local_size_y,
                      size_t local_size_z) = 0;

  /// @brief Creates a binary loadable by muxCreateExecutable containing (at
  /// least) this kernel, possibly optimized with a specific configuration. This
  /// function provides an opportunity to defer compilation of kernels until
  /// enqueue time.
  ///
  /// @param specialization_options Mux execution options to specialize for.
  ///
  /// @return A valid binary object if specialization was successful,
  /// or a status code otherwise.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_VALUE` if any of the specialization options are
  /// invalid.
  /// @retval `Result::FINALIZE_PROGRAM_FAILURE` if there was a failure to
  /// create the specialized kernel.
  virtual cargo::expected<cargo::dynamic_array<uint8_t>, Result>
  createSpecializedKernel(
      const mux_ndrange_options_t &specialization_options) = 0;

  /// @brief Returns the sub-group size for this kernel.
  ///
  /// This function queries a kernel for maximum sub-group size that would exist
  /// in a kernel enqueued with the local work-group size passed as parameters.
  /// A kernel enqueue **may** include one sub-group with a smaller size when
  /// the sub-group size doesn't evenly divide the local size.
  ///
  /// @param[in] local_size_x Local size in x dimension of the work-group for
  /// which the sub-group count is being queried.
  /// @param[in] local_size_y Local size in y dimension of the work-group for
  /// which the sub-group count is being queried.
  /// @param[in] local_size_z Local size in z dimension of the work-group for
  /// which the sub-group count is being queried.
  ///
  /// @return Returns the sub-group size for the kernel, or a status code if it
  /// was unsuccessful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::FEATURE_UNSUPPORTED` if sub-groups are not supported by
  /// this kernel.
  virtual cargo::expected<uint32_t, Result>
  querySubGroupSizeForLocalSize(size_t local_size_x, size_t local_size_y,
                                size_t local_size_z) = 0;

  /// @brief Calculates the local size that would return the requested sub-group
  /// size.
  ///
  /// @param[in] sub_group_size The requested sub-group size for the kernel.
  ///
  /// @return Returns the local size that would result in sub-groups of size
  /// `sub_group_size`, this local size **must** be 1D i.e. at least two of the
  /// elements **must** be 1. **May** return {0, 0, 0} in the case no local size
  /// would result in the requested sub-group size. Returns a status code if the
  /// query was unsuccessful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::FEATURE_UNSUPPORTED` if sub-groups are not supported by
  /// this kernel.
  virtual cargo::expected<std::array<size_t, 3>, Result>
  queryLocalSizeForSubGroupCount(size_t sub_group_count) = 0;

  /// @brief Returns the maximum number of sub-groups the kernel can support in
  /// an enqueue.
  ///
  /// In general this will be a function of the device, the sub-group
  /// implementation and the content of the kernel.
  ///
  /// @return Maximum number of sub-groups the kernel can support, or a status
  /// code if it was unsuccessful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::FEATURE_UNSUPPORTED` if sub-groups are not supported by
  /// this kernel.
  virtual cargo::expected<size_t, Result> queryMaxSubGroupCount() = 0;

  /// @brief The name of the kernel.
  const std::string name;

  /// @brief The preferred local size in the x dimension for this kernel.
  const size_t preferred_local_size_x;

  /// @brief The preferred local size in the y dimension for this kernel.
  const size_t preferred_local_size_y;

  /// @brief The preferred local size in the z dimension for this kernel.
  const size_t preferred_local_size_z;

  /// @brief The amount of local memory used by this kernel.
  const size_t local_memory_size;
};

/// @}
} // namespace compiler

#endif // COMPILER_KERNEL_H_INCLUDED
