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
/// @brief Implementation of `cl_codeplay_wfv` extension.

#ifndef EXTENSION_CODEPLAY_WFV_H_INCLUDED
#define EXTENSION_CODEPLAY_WFV_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of cl_codeplay_wfv extension.
class codeplay_wfv final : public extension {
 public:
  /// @brief Default constructor.
  codeplay_wfv();

  /// @brief Queries for the extension function associated with func_name.
  ///
  /// If extension is enabled, then makes an extensions function query-able:
  /// * `clGetKernelWFVInfoCODEPLAY`
  ///
  /// @see clGetExtensionFunctionAddressForPlatform.
  ///
  /// @param[in,out] platform OpenCL platform func_name belongs to.
  /// @param[in] func_name name of the extension function to query for.
  /// Supported function name if extension is enabled:
  /// * "clGetKernelWFVInfoCODEPLAY"
  /// @return Returns a pointer to the extension function or nullptr if no
  /// function with the name func_name exists.
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;

  /// @copydoc extension::extension::GetDeviceInfo
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;
};

/// @}
}  // namespace extension

namespace wfv {
/// @brief Checks if an OpenCL device can support vectorization, a mandatory
/// feature of the extension specification.
///
/// @param[in] device OpenCL device to query support for.
///
/// @return True if device can support vectorization, false otherwise.
bool deviceSupportsVectorization(cl_device_id device);
}  // namespace wfv

#endif  // EXTENSION_CODEPLAY_WFV_H_INCLUDED
