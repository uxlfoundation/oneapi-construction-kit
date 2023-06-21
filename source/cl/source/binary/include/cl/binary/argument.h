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
/// @brief Extract OpenCL metadata from kernels.

#ifndef CL_BINARY_ARGUMENT_H_INCLUDED
#define CL_BINARY_ARGUMENT_H_INCLUDED

#include <cargo/string_view.h>
#include <compiler/module.h>

#include <cassert>
#include <cstdint>
#include <string>

namespace cl {
namespace binary {

/// @brief Convert a parameter type string into a ArgumentType.
///
/// Pointers and `structByVal` types are not supported.
///
/// @param[in] str string_view containing parameter info excluding name.
///
/// @return An ArgumentType description of the parameter.
std::pair<compiler::ArgumentType, std::string>
getArgumentTypeFromParameterTypeString(const cargo::string_view &str);

}  // namespace binary
}  // namespace cl

#endif  // CL_BINARY_ARGUMENT_H_INCLUDED
