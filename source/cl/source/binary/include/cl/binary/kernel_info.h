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

#ifndef CL_BINARY_KERNEL_INFO_H_INCLUDED
#define CL_BINARY_KERNEL_INFO_H_INCLUDED

#include <CL/cl.h>
#include <cargo/dynamic_array.h>
#include <cargo/optional.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <cl/binary/argument.h>

#include <string>

namespace cl {
namespace binary {

/// @brief Class for storing kernel info.
class KernelInfo {
 public:
  /// @brief Default constructor.
  KernelInfo();

  /// @brief Struct representing basic kernel argument information.
  struct ArgumentInfo final {
    ArgumentInfo();

    cl_kernel_arg_address_qualifier address_qual;
    cl_kernel_arg_access_qualifier access_qual;
    cl_kernel_arg_type_qualifier type_qual;
    std::string type_name;
    std::string name;
  };  // struct ArgumentInfo

  std::string name;
  std::string attributes;
  uint32_t num_arguments;
  cargo::dynamic_array<ArgumentType> argument_types;
  cargo::optional<cargo::small_vector<ArgumentInfo, 8>> argument_info;
  uint64_t private_mem_size;

  /// @brief Values of reqd_work_group_size attribute.
  ///
  /// @remark *** Attention ***
  /// Work_group should only be set if reqd_work_group_size attribute is
  /// specified! Code in clEnqueueNDRangeKernel and clEnqueueTask relies on this
  /// assumption.
  std::array<size_t, 3> work_group;
};  // class KernelInfo

/// @brief Extract kernel information from a declaration string.
///
/// This method is intended for parsing built-in kernel declaration strings
/// that have been hard-coded into the driver. It must not be used to parse
/// strings originating from OpenCL API calls. For performance reasons,
/// strings are assumed to be syntactically correct, and error conditions are
/// only checked for in assert builds.
///
/// Errors may leave the KernelInfo object in an undefined state in builds
/// without assertions.
///
/// The built-in kernel declaration syntax is defined in the "Built-In Kernel
/// Declaration Syntax" section of the Core Specification.
///
/// Note: We pass a `KernelInfo` by reference here, as `KernelInfo` objects are
/// already allocated and contained inside a `ProgramInfo` object, so it makes
/// more sense to "fill it in" rather than allocate a new object and replace the
/// previous one.
///
/// @param[in] kernel_info KernelInfo to populate.
/// @param[in] decl string_view of the declaration.
/// @param[in] store_arg_metadata Whether to store additional argument
/// metadata as required by -cl-kernel-arg-info.
///
/// @return Return true if successful, false on allocation failures.
bool kernelDeclStrToKernelInfo(KernelInfo &kernel_info,
                               const cargo::string_view decl,
                               bool store_arg_metadata);

}  // namespace binary
}  // namespace cl

#endif  // CL_BINARY_KERNEL_INFO_H_INCLUDED
