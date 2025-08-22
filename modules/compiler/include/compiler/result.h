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
/// @brief Compiler result enumeration.

#ifndef COMPILER_RESULT_H_INCLUDED
#define COMPILER_RESULT_H_INCLUDED

#include <cstddef>
#include <cstdint>

namespace compiler {
/// @addtogroup compiler
/// @{

enum class Result {
  /// @brief No error occurred.
  SUCCESS,
  /// @brief An unknown error occurred.
  FAILURE,
  /// @brief A invalid value was passed to a method.
  INVALID_VALUE,
  /// @brief A memory allocation failed.
  OUT_OF_MEMORY,
  /// @brief Invalid build options were passed to the frontend.
  INVALID_BUILD_OPTIONS,
  /// @brief Invalid compile options were passed to the frontend.
  INVALID_COMPILER_OPTIONS,
  /// @brief Invalid link options were passed to the frontend.
  INVALID_LINKER_OPTIONS,
  /// @brief Failed to compile the program in BUILD mode.
  BUILD_PROGRAM_FAILURE,
  /// @brief Failed to compile the program in COMPILE mode.
  COMPILE_PROGRAM_FAILURE,
  /// @brief Failed to link the program.
  LINK_PROGRAM_FAILURE,
  /// @brief Failed to finalize the program i.e. transform the LLVM module into
  /// a binary.
  FINALIZE_PROGRAM_FAILURE,
  /// @brief Indicates a feature isn't supported.
  FEATURE_UNSUPPORTED,
};

/// @}
} // namespace compiler

#endif // COMPILER_RESULT_H_INCLUDED
