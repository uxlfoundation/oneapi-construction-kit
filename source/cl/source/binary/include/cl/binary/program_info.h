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
/// @brief Compiler program info API.

#ifndef CL_BINARY_PROGRAM_INFO_H_INCLUDED
#define CL_BINARY_PROGRAM_INFO_H_INCLUDED

#include <cargo/optional.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <cl/binary/kernel_info.h>

namespace cl {
namespace binary {

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
cargo::optional<compiler::ProgramInfo> kernelDeclsToProgramInfo(
    const cargo::small_vector<std::string, 8> &decls, bool store_arg_metadata);

}  // namespace binary
}  // namespace cl

#endif  // CL_BINARY_PROGRAM_INFO_H_INCLUDED
