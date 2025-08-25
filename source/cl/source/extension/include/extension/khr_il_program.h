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
/// @brief Intermediate Language Programs.
///
/// This extension adds support for creating programs with intermediate
/// language (usually SPIR-V). For further information about the format and
/// contents of SPIR-V, refer to the SPIR-V specification. For information
/// about the OpenCL 1.2 SPIR-V capabilities required of the OpenCL runtime,
/// refer to chapter 6 of the SPIR-V OpenCL environment specification.
///
/// The name of this extension is `cl_khr_il_program`.

#ifndef CL_EXTENSION_KHR_IL_PROGRAM_H_INCLUDED
#define CL_EXTENSION_KHR_IL_PROGRAM_H_INCLUDED

#include <cargo/array_view.h>
#include <cl/program.h>
#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

class khr_il_program : public extension {
 public:
  khr_il_program();

  /// @brief Queries for the extension function associated with `func_name`.
  ///
  /// New procedures and functions: `clCreateProgramWithILKHR`.
  ///
  /// @see Section 9.21.2 of
  /// https://www.khronos.org/registry/OpenCL/specs/opencl-1.2-extensions.pdf
  ///
  /// @param[in] platform OpenCL platform `func_name` belongs to.
  /// @param[in] func_name Name of the extension function to query for.
  ///
  /// @return Returns a pointer to the extension function with `func_name` or
  /// `nullptr` if it not exist.
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;

  /// @brief Queries for extension provided device info.
  ///
  /// Add new device property: `CL_DEVICE_IL_VERSION_KHR`
  ///
  /// The intermediate languages that can be supported by
  /// `clCreateProgramWithILKHR` for this device.
  ///
  /// Set to a space separated list of IL version strings of the form
  /// `<IL_Prefix>_<Major_version>.<Minor_version>` "SPIR-V" is a required IL
  /// prefix when the `cl_khr_il_program` extension is reported.
  ///
  /// @see Section 9.21.5 of
  /// https://www.khronos.org/registry/OpenCL/specs/opencl-1.2-extensions.pdf
  ///
  /// @param[in] device OpenCL device to query.
  /// @param[in] param_name Information to query for.
  /// @param[in] param_value_size Size in bytes of the memory area
  /// `param_value` points to.
  /// @param[in] param_value Memory area to copy the queried info in or
  /// `nullptr` to not receive the info value.
  /// @param[out] param_value_size_ret References variable where to store the
  /// size of the info value or `nullptr` to not receive the info value.
  ///
  /// @return Returns an OpenCL error code.
  /// @retval CL_SUCCESS when the requested property was returned.
  /// @retval CL_INVALID_VALUE when 1.) `param_value_size` is `0` and
  /// `param_value` is not `nullptr`, or 2.) `param_value_size` is too small.
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;

  /// @brief Query for program info.
  ///
  /// Add new program property: `CL_PROGRAM_IL_KHR`
  ///
  /// Returns the program IL for programs created with
  /// `clCreateProgramWithILKHR`.
  ///
  /// If program is created with `clCreateProgramWithSource`,
  /// `clCreateProgramWithBinary`, or `clCreateProgramWithBuiltinKernels`, the
  /// memory pointed to by `param_value` will be unchanged and
  /// `param_value_size_ret` will be set to zero.
  ///
  /// @see Section 9.21.10 of
  /// https://www.khronos.org/registry/OpenCL/specs/opencl-1.2-extensions.pdf
  ///
  /// @param[in] program OpenCL program to query.
  /// @param[in] param_name Specific information to query for.
  /// @param[in] param_value_size Size of memory area pointed to by param_value.
  /// Can be 0 if param_value is nullptr.
  /// @param[out] param_value Memory area to store the query result in.
  /// @param[out] param_value_size_ret Points to memory area to store the
  /// minimally required param_value size in. Can be nullptr.
  ///
  /// @return Returns an OpenCL error code.
  /// @retval CL_SUCCESS when the requested property was returned.
  /// @retval CL_INVALID_VALUE when 1.) `param_value_size` is `0` and
  /// `param_value` is not `nullptr`, or 2.) `param_value_size` is too small.
  cl_int GetProgramInfo(cl_program program, cl_program_info param_name,
                        size_t param_value_size, void *param_value,
                        size_t *param_value_size_ret) const override;
};

/// @}
}  // namespace extension

#endif  // CL_EXTENSION_KHR_IL_PROGRAM_H_INCLUDED
