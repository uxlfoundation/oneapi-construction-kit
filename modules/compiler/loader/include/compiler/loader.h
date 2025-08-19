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
#ifndef COMPILER_LOADER_H_INCLUDED
#define COMPILER_LOADER_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/expected.h>
#include <cargo/optional.h>
#include <mux/mux.hpp>

#include <memory>
#include <string>

namespace compiler {
/// @brief Library forward declaration.
struct Library;
}  // namespace compiler

// When using the template std::unique_ptr<Library> where Library is an
// incomplete type, we encounter compiler errors as the non-specialized
// definition of std::default_delete<T> does not work for incomplete types.
//
// To avoid this error, we provide an explicit specialization of
// std::default_delete for compiler::Library that declares operator(), which we
// define later on in a source file.
namespace std {
template <>
struct default_delete<compiler::Library> {
  default_delete() noexcept = default;
  void operator()(compiler::Library *library) const;
};
}  // namespace std

namespace compiler {
/// @addtogroup compiler
/// @{

/// @brief Info forward declaration.
struct Info;

/// @brief Context forward declaration.
class Context;

/// @brief Loads the compiler library.
///
/// @return An instance of the compiler library if it exists. If there is no
/// compiler library available, the return value will be nullptr. If there was
/// a compiler library available, but the library was invalid, than an error
/// message will be returned.
cargo::expected<std::unique_ptr<Library>, std::string> loadLibrary();

/// @brief Queries the LLVM version used by the compiler. If LLVM was built with
/// a build type other than "Release", the build type will also be appended to
/// the string. Examples include: "11.0.1" or "12.0.0 (Debug)".
///
/// @param[in] library Compiler library.
///
/// @return The LLVM version string that the compiler was built with. This
/// will always return a valid string.
const char *llvmVersion(Library *library);

/// @brief Returns a list of all supported compilers.
///
/// @param[in] library Compiler library.
///
/// @return Returns a list of static `compiler::Info` instances containing all
/// compilers which are available for use.
cargo::array_view<const compiler::Info *> compilers(Library *library);

/// @brief Returns a compiler info that compiles binaries for a given Mux
/// device.
///
/// @param[in] library Compiler library.
/// @param[in] device_info Mux device info associated with the desired compiler
/// instance.
///
/// @return Returns an instance of `compiler::Info` if this particular
/// device_info has a compiler associated with it. If not, then `nullptr` will
/// be returned.
const compiler::Info *getCompilerForDevice(Library *library,
                                           mux_device_info_t device_info);

/// @brief Returns a new compiler context. This context must be destroyed using
/// `delete`.
///
/// @param[in] library Compiler library.
///
/// @return A new compiler context. If `compiler` is `nullptr`, then `nullptr`
/// will be returned.
std::unique_ptr<compiler::Context> createContext(Library *library);

/// @}
}  // namespace compiler

#endif  // COMPILER_LOADER_H_INCLUDED
