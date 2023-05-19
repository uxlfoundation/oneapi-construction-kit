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
/// @brief Compiler program info API.

#ifndef CL_BINARY_PROGRAM_INFO_H_INCLUDED
#define CL_BINARY_PROGRAM_INFO_H_INCLUDED

#include <cargo/optional.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <cl/binary/kernel_info.h>

namespace cl {
namespace binary {

/// @brief Class for managing program information.
class ProgramInfo {
 public:
  ProgramInfo();

  /// @brief Add a single empty kernel info for later population.
  bool addNewKernel();

  /// @brief Initialize empty program information for a specified number of
  /// kernels for later population.
  ///
  /// @param[in] numKernels Number of kernels to allocate space for.
  bool resizeFromNumKernels(int32_t numKernels);

  /// @brief  Retrieve the number of kernels.
  ///
  /// @return Return the number of kernels.
  inline size_t getNumKernels() const { return kernel_descriptions.size(); }

  /// @brief Retrieve a kernel by index.
  ///
  /// @param[in] kernel_index Index into the list of kernel infos.
  ///
  /// @return Return kernel info if found, null otherwise.
  KernelInfo *getKernel(size_t kernel_index);

  /// @brief Retrieve a kernel by index.
  ///
  /// @param[in] kernel_index Index into the list of kernel infos.
  ///
  /// @return Return kernel info if found, null otherwise.
  const KernelInfo *getKernel(size_t kernel_index) const;

  /// @brief Retrieve a kernel by name.
  ///
  /// @param[in] kernel_name Name of the kernel to search for.
  ///
  /// @return Return kernel info if found, null otherwise.
  KernelInfo *getKernelByName(cargo::string_view kernel_name);

  /// @brief Retrieve a kernel by name.
  ///
  /// @param[in] kernel_name Name of the kernel to search for.
  ///
  /// @return Return kernel info if found, null otherwise.
  const KernelInfo *getKernelByName(cargo::string_view kernel_name) const;

  /// @brief Retrieve the begin iterator.
  ///
  /// @return The beginning of the kernel info range.
  KernelInfo *begin();

  /// @brief Retrieve the begin iterator.
  ///
  /// @return The beginning of the kernel info range.
  const KernelInfo *begin() const;

  /// @brief Retrieve the end iterator.
  ///
  /// @return Return the end iterator.
  KernelInfo *end();

  /// @brief Retrieve the end iterator.
  ///
  /// @return Return the end iterator.
  const KernelInfo *end() const;

 private:
  /// @brief Kernel descriptions.
  cargo::small_vector<KernelInfo, 8> kernel_descriptions;
};  // class ProgramInfo

/// @brief Initialize information for all built-in kernels from their
/// declarations.
///
/// @param[in] decls Vector of strings, where each string is a built-in kernel
/// declaration.
/// @param[in] store_arg_metadata Whether to store additional argument
/// metadata as required by -cl-kernel-arg-info.
///
/// @return Returns a valid ProgramInfo if successful, or cargo::nullopt
/// otherwise.
cargo::optional<ProgramInfo> kernelDeclsToProgramInfo(
    const cargo::small_vector<std::string, 8> &decls, bool store_arg_metadata);

}  // namespace binary
}  // namespace cl

#endif  // CL_BINARY_PROGRAM_INFO_H_INCLUDED
