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

#ifndef CL_BINARY_SPIRV_H_INCLUDED
#define CL_BINARY_SPIRV_H_INCLUDED

#include <cargo/string_view.h>
#include <compiler/module.h>
#include <mux/mux.h>

namespace cl {
namespace binary {

/// @brief Get device specific information needed to compile a SPIR-V module.
///
/// @param device_info Target device information to compile the module.
/// @param profile The OpenCL profile to target.
///
/// @return Returns device specific SPIR-V information, or a cargo failure if
/// there was an error.
cargo::expected<compiler::spirv::DeviceInfo, cargo::result> getSPIRVDeviceInfo(
    mux_device_info_t device_info, cargo::string_view profile);

}  // namespace binary
}  // namespace cl

#endif  // CL_BINARY_SPIRV_H_INCLUDED
