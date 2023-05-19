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

/// @brief Enumeration of standard OpenCL address space values.
enum AddressSpace : uint32_t {
  PRIVATE = 0,
  GLOBAL = 1,
  CONSTANT = 2,
  LOCAL = 3,
};

/// @brief Struct to hold type and related metadata.
struct ArgumentType final {
  /// @brief Default constructor.
  ArgumentType()
      : kind(compiler::ArgumentKind::UNKNOWN),
        address_space(0),
        type_name_str(),
        vector_width(1),
        is_other(false),
        dereferenceable_bytes() {}

  /// @brief Non-pointer constructor.
  ///
  /// @param kind The argument kind.
  ArgumentType(compiler::ArgumentKind kind)
      : kind(kind),
        address_space(0),
        type_name_str(),
        vector_width(1),
        is_other(false),
        dereferenceable_bytes() {}

  /// @brief Pointer constructor.
  ///
  /// @param address_space The address space of the pointer argument.
  ArgumentType(uint32_t address_space)
      : kind(compiler::ArgumentKind::POINTER),
        address_space(address_space),
        type_name_str(),
        vector_width(1),
        is_other(false),
        dereferenceable_bytes() {}

  /// @brief Default destructor.
  ~ArgumentType() = default;

  /// @brief The type (possibly including integer bit width and vector width).
  compiler::ArgumentKind kind;
  /// @brief The address space of the argument.
  uint32_t address_space;
  /// @brief Type name string provided by user (including vector width).
  ///
  /// Will start with a `u` if the type is unsigned. This is the only place
  /// where unsigned information is stored.
  ///
  /// This is the string that gets reported by @a CL_KERNEL_ARG_TYPE_NAME.
  std::string type_name_str;
  /// @brief Vector width of type (only relevant to integer and fp types).
  size_t vector_width;
  /// @brief If the type is an `image*` or `sampler_t` type. Storing this
  /// information here makes parsing easier later.
  bool is_other;
  /// @brief returns the amount of deferencable bytes of the argument. Note: The
  /// argument must be a pointer type
  cargo::optional<uint64_t> dereferenceable_bytes;
};

/// @brief Convert a parameter type string into a ArgumentType.
///
/// Pointers and `structByVal` types are not supported.
///
/// @param[in] str string_view containing parameter info excluding name.
///
/// @return An ArgumentType description of the parameter.
ArgumentType getArgumentTypeFromParameterTypeString(
    const cargo::string_view &str);

}  // namespace binary
}  // namespace cl

#endif  // CL_BINARY_ARGUMENT_H_INCLUDED
