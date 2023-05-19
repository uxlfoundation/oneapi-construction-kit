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
/// @brief Compiler target API.

#ifndef COMPILER_TARGET_H_INCLUDED
#define COMPILER_TARGET_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/string_view.h>
#include <compiler/info.h>
#include <compiler/result.h>
#include <mux/mux.h>

#include <map>

namespace compiler {
/// @addtogroup compiler
/// @{

/// @brief Module forward declaration.
class Module;

/// @brief Context forward declaration.
class Context;

/// @brief Embedded builtins file capabilities
enum BuiltinsCapabilities : uint32_t {
  /// @brief Default (minimal) capabilities (64-bit).
  CAPS_DEFAULT = 0x0,
  /// @brief 32-bit file (default is 64-bit).
  CAPS_32BIT = 0x1,
  /// @brief File with floating point double types.
  CAPS_FP64 = 0x2,
  /// @brief File with floating point half types.
  CAPS_FP16 = 0x4,
};

/// @brief Compiler target class.
class Target {
 public:
  /// @brief Virtual destructor.
  virtual ~Target() = default;

  /// @brief Initialize the compiler target.
  ///
  /// @param[in] builtins_capabilities Capabilities for the embedded builtins.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when initialization was successful.
  /// @retval `Result::INVALID_VALUE` if `builtins_capabilities` contains any
  /// invalid capabilities.
  /// @retval `Result::FAILURE` if any other failure occurred.
  virtual Result init(uint32_t builtins_capabilities) = 0;

  /// @brief List all the snapshot stages available.
  ///
  /// @param[in] count Element count of the `out_stages` array, must be
  /// greater than zero if `out_stages` is not NULL and zero otherwise.
  /// @param[out] out_stages Array of C strings to be populated with snapshot
  /// names, may be NULL. Must have at least `count` elements.
  /// @param[out] out_count Number of snapshot stages available, may be
  /// NULL.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when initialization was successful.
  /// @retval `Result::INVALID_VALUE` if `count` is 0, and `out_stages` is not
  /// NULL.
  /// @retval `Result::INVALID_VALUE` if `out_stages` is `nullptr`, and `count`
  /// is not 0.
  virtual Result listSnapshotStages(uint32_t count, const char **out_stages,
                                    uint32_t *out_count) = 0;

  /// @brief Creates a compiler module targeting this compiler target.
  ///
  /// @param[in,out] num_errors Reference to a variable which will store the
  /// number of errors occurred during compilation.
  /// @param[in,out] log Reference to a string which will store the log of the
  /// compiler.
  ///
  /// @return A new compiler module.
  virtual std::unique_ptr<compiler::Module> createModule(uint32_t &num_errors,
                                                         std::string &log) = 0;

  /// @brief Returns the compiler info associated with this target.
  virtual const compiler::Info *getCompilerInfo() const = 0;

};  // class Target

/// @}
}  // namespace compiler

#endif  // COMPILER_TARGET_H_INCLUDED
