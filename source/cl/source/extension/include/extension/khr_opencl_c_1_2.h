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
/// @brief Extension cl_khr_opencl_c_1_2 API.

#ifndef EXTENSION_KHR_OPENCL_C_1_2_H_INCLUDED
#define EXTENSION_KHR_OPENCL_C_1_2_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Default extensions associated with OpenCL 1.2.
///
/// If enabled, then ClGetPlatformInfo queries for CL_PLATFORM_EXTENSIONS return
/// the following names as the query value:
/// * `cl_khr_global_int32_base_atomics`
/// * `cl_khr_global_int32_extended_atomics`
/// * `cl_khr_local_int32_base_atomics`
/// * `cl_khr_local_int32_extended_atomics`
/// * `cl_khr_byte_addressable_store`
/// * `cl_khr_fp64` is only included if the queried device supports double
///   precision floating point types.
class khr_opencl_c_1_2 final : public extension {
 public:
  /// @brief Default constructor.
  khr_opencl_c_1_2();

  /// @brief Queries for extension provided device info.
  ///
  /// If extension is enabled, then ClGetDeviceInfo queries for
  /// CL_DEVICE_EXTENSIONS return the following query value:
  /// * `cl_khr_global_int32_base_atomics`
  /// * `cl_khr_global_int32_extended_atomics`
  /// * `cl_khr_local_int32_base_atomics`
  /// * `cl_khr_local_int32_extended_atomics`
  /// * `cl_khr_byte_addressable_store`
  /// * `cl_khr_fp64` is only included if the queried device supports double
  ///   precision floating point types.
  ///
  /// @see ocl::extension::Extension::ClGetDeviceInfo for more detailed
  /// explanations.
  ///
  /// @param[in] device OpenCL device to query.
  /// @param[in] param_name Information to query for. If extension is enabled,
  /// then ClGetDeviceInfo queries for CL_DEVICE_EXTENSIONS return the names
  /// listed above.
  /// @param[in] param_value_size Size in bytes of the memory area param_value
  /// points to.
  /// @param[in] param_value Memory area to copy the queried info in or nullptr
  /// to not receive the info value.
  /// @param[out] param_value_size_ret References variable where to store the
  /// size of the info value or nullptr to not receive the info value.
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;
};

/// @}
}  // namespace extension

#endif  // EXTENSION_KHR_OPENCL_C_1_2_H_INCLUDED
