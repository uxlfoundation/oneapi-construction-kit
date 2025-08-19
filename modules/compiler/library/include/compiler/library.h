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
#ifndef COMPILER_LIBRARY_H_INCLUDED
#define COMPILER_LIBRARY_H_INCLUDED

#include <cargo/array_view.h>
#include <mux/mux.hpp>

#include <memory>

namespace compiler {
/// @addtogroup compiler
/// @{

/// @brief Info forward declaration.
struct Info;

/// @brief Context forward declaration.
class Context;

/// @brief Queries the LLVM version used by the compiler. If LLVM was built with
/// a build type other than "Release", the build type will also be appended to
/// the string. Examples include: "11.0.1" or "12.0.0 (Debug)".
///
/// @return The LLVM version string that the compiler was built with. This
/// will always return a valid string.
const char *llvmVersion();

/// @brief Returns a list of all supported compilers.
///
/// @return Returns a list of static `compiler::Info` instances containing all
/// compilers which are available for use.
cargo::array_view<const compiler::Info *> compilers();

/// @brief Returns a compiler info that compiles binaries for a given Mux
/// device.
///
/// @param[in] device_info Mux device info associated with the desired compiler
/// instance.
///
/// @return Returns an instance of `compiler::Info` if this particular
/// device_info has a compiler associated with it. If not, then `nullptr` will
/// be returned.
const compiler::Info *getCompilerForDevice(mux_device_info_t device_info);

/// @brief Returns a new compiler context. This context must be destroyed using
/// `delete`.
///
/// @return A new compiler context.
std::unique_ptr<compiler::Context> createContext();

/// @}
}  // namespace compiler

#endif  // COMPILER_LIBRARY_H_INCLUDED
