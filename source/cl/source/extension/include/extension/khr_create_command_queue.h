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
/// @brief Support for the `cl_khr_create_command_queue` extension.

#ifndef EXTENSION_KHR_CREATE_COMMAND_QUEUE_H_INCLUDED
#define EXTENSION_KHR_CREATE_COMMAND_QUEUE_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

class khr_create_command_queue final : public extension {
 public:
  /// @brief Default constructor.
  khr_create_command_queue();

  /// @brief Queries for the extension function associated with func_name.
  ///
  /// If extension is enabled, then makes an extensions function query-able:
  /// * `clCreateCommandQueueWithPropertiesKHR`
  ///
  /// @see `cl::GetExtensionFunctionAddressForPlatform`.
  ///
  /// @param[in,out] platform OpenCL platform func_name belongs to.
  /// @param[in] func_name name of the extension function to query for.
  /// Supported function name if extension is enabled:
  /// * `clCreateCommandQueueWithPropertiesKHR`
  ///
  /// @return Returns a pointer to the extension function or nullptr if no
  /// function with the name func_name exists.
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;
};

/// @}
}  // namespace extension

#endif  // EXTENSION_KHR_CREATE_COMMAND_QUEUE_H_INCLUDED
