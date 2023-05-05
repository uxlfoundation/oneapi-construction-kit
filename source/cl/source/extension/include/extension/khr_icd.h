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
/// @brief Functions for cl_khr_icd extension.

#ifndef CL_KHR_CL_ICD_H_INCLUDED
#define CL_KHR_CL_ICD_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief cl_khr_icd extension.
///
/// If enabled, then `GetPlatformInfo` queries for `CL_PLATFORM_EXTENSIONS`
/// return `"cl_khr_icd"` as the query value.
///
/// If enabled makes an extensions function query-able via
/// `clGetExtensionFunctionAddressForPlatform`: * clIcdGetPlatformIDsKHR
class khr_icd final : public extension {
 public:
  /// @brief Default constructor.
  khr_icd();

  /// @brief Queries for extension provided platform info.
  ///
  /// If extension is enabled, then `GetPlatformInfo` queries for
  /// `CL_PLATFORM_EXTENSIONS` return `"cl_khr_icd"` as the query value.
  ///
  /// @see ::extension::extension::GetPlatformInfo for more detailed
  /// explanations.
  ///
  /// @param[in] platform OpenCL platform to query.
  /// @param[in] param_name Information to query for. If extension is enabled,
  /// then ClGetPlatformInfo queries for CL_PLATFORM_EXTENSIONS return
  /// "cl_khr_icd" as the query value.
  /// @param[in] param_value_size Size in bytes of the memory area param_value
  /// points to.
  /// @param[in] param_value Memory area to copy the queried info in or nullptr
  /// to not receive the info value.
  /// @param[out] param_value_size_ret References variable where to store the
  /// size of the info value or nullptr to not receive the info value.
  ///
  /// @return Returns an OpenCL error code.
  /// @retval `CL_SUCCESS` if the extension accepts the param_name query
  /// and the supplied argument values.
  /// @retval `CL_INVALID_VALUE` if the extension does not accept the
  /// `param_name` query.
  cl_int GetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                         size_t param_value_size, void *param_value,
                         size_t *param_value_size_ret) const override;

  /// @brief Queries for the extension function associated with func_name.
  ///
  /// If extension is enabled, then makes an extensions function query-able:
  /// * `clIcdGetPlatformIDsKHR`
  ///
  /// @see `cl::GetExtensionFunctionAddressForPlatform`.
  ///
  /// @param[in,out] platform OpenCL platform func_name belongs to.
  /// @param[in] func_name name of the extension function to query for.
  /// Supported function name if extension is enabled:
  /// * `clIcdGetPlatformIDsKHR`
  ///
  /// @return Returns a pointer to the extension function or nullptr if no
  /// function with the name func_name exists.
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;

  /// @brief Returns a pointer to the ICD function pointer dispatch table.
  ///
  /// @return Pointer to the dispatch table
  static const void *GetIcdDispatchTable();
};

/// @}
}  // namespace extension

#endif  // CL_KHR_CL_ICD_H_INCLUDED
