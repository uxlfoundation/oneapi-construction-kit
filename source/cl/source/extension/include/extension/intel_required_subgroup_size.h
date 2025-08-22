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
/// @brief Functions for cl_intel_required_subgroup_size extension.

#ifndef EXTENSION_INTEL_REQUIRED_SUBGROUP_SIZE_H_INCLUDED
#define EXTENSION_INTEL_REQUIRED_SUBGROUP_SIZE_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of the cl_intel_required_subgroup_size extension.
class intel_required_subgroup_size final : public extension {
public:
  /// @brief Default constructor.
  intel_required_subgroup_size();

  /// @brief Queries for extension provided device info.
  ///
  /// If enabled, then `GetDeviceInfo` queries for `CL_DEVICE_EXTENSIONS` return
  /// `"cl_intel_required_subgroup_size"` as the query value. Queries for
  /// `CL_DEVICE_SUB_GROUP_SIZES_INTEL` return an array of `size_t` values with
  /// the supported sub-group sizes.
  ///
  /// @see `::extension::extension::GetDeviceInfo` for more detailed
  /// explanations.
  ///
  /// @param[in] device OpenCL device to query.
  /// @param[in] param_name Information to query for, e.g.:
  /// * CL_DEVICE_EXTENSIONS
  /// * CL_DEVICE_SUB_GROUP_SIZES_INTEL
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
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;

  /// @brief Query for extension provided kernel work group info.
  ///
  /// @see `::extension::extension::GetKernelWorkGroupInfo` for more detailed
  /// explanations.
  ///
  /// @param[in] kernel OpenCL kernel to query.
  /// @param[in] device Specific device to take into account for kernel work
  /// group information.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int
  GetKernelWorkGroupInfo(cl_kernel kernel, cl_device_id device,
                         cl_kernel_work_group_info param_name,
                         size_t param_value_size, void *param_value,
                         size_t *param_value_size_ret) const override;

#if defined(CL_VERSION_3_0)
  /// @brief Query for extension provided kernel subgroup info.
  ///
  /// @see `::extension::extension::GetKernelSubGroupInfo` for more detailed
  /// explanations.
  ///
  /// @param[in] kernel OpenCL kernel to query.
  /// @param[in] device Identifies device in device list associated with @p
  /// kernel.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] input_value_size Size in bytes of memory pointed to by
  /// input_value.
  /// @param[in] input_value Pointer to memory where parameterization of query
  /// is passed from.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns CL_SUCCESS if the extension accepts the param_name query
  /// and the supplied argument values. CL_INVALID_VALUE if the extension does
  /// not accept the param_name query. Other OpenCL error return code if the
  /// extension accepts param_name but the supplied argument values are wrong.
  virtual cl_int
  GetKernelSubGroupInfo(cl_kernel kernel, cl_device_id device,
                        cl_kernel_sub_group_info param_name,
                        size_t input_value_size, const void *input_value,
                        size_t param_value_size, void *param_value,
                        size_t *param_value_size_ret) const override;
#endif
};

/// @}
} // namespace extension

#endif // EXTENSION_INTEL_REQUIRED_SUBGROUP_SIZE_H_INCLUDED
