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
/// @brief Compiler context API.

#ifndef COMPILER_CONTEXT_H_INCLUDED
#define COMPILER_CONTEXT_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/expected.h>

#include <mutex>
#include <unordered_map>

namespace compiler {
/// @addtogroup compiler
/// @{

namespace spirv {
/// @brief Enumeration of SPIR-V constant types which can be specialized.
enum class SpecializationType : uint8_t {
  BOOL,   ///< OpTypeBool specialization constant.
  INT,    ///< OpTypeInt specialization constant.
  FLOAT,  ///< OpTypeFloat specialization constant.
};

/// @brief Description of a SPIR-V constant which can be specialized.
struct SpecializationDesc {
  /// @brief Type of the specializable constant.
  SpecializationType constant_type;
  /// @brief Size in bits of the specializable constant.
  uint32_t size_in_bits;
};

/// @brief Type for mapping SPIR-V specializaton constant ID to it's
/// description.
using SpecializableConstantsMap =
    std::unordered_map<std::uint32_t, SpecializationDesc>;
}  // namespace spirv

/// @brief Compiler context class.
///
/// Satisfies the C++ named requirement 'Lockable' so it can be used as part
/// of std::lock_guard or std::unique_lock.
class Context {
 public:
  /// @brief Virtual destructor.
  virtual ~Context() = default;

  /// @brief Checks if a binary stream is valid SPIR-V.
  ///
  /// @param code View of the SPIR-V binary stream.
  ///
  /// @return Returns `true` if the stream is valid, `false` otherwise.
  virtual bool isValidSPIRV(cargo::array_view<const uint32_t> code) = 0;

  /// @brief Get a description of all of a SPIR-V modules specializable
  /// constants.
  ///
  /// @param code View of the SPIR-V binary stream.
  ///
  /// @return Returns a map of the modules specializable constants on success,
  /// otherwise returns an error string.
  virtual cargo::expected<spirv::SpecializableConstantsMap, std::string>
  getSpecializableConstants(cargo::array_view<const uint32_t> code) = 0;
};  // class Context
/// @}
}  // namespace compiler

#endif  // COMPILER_CONTEXT_H_INCLUDED
