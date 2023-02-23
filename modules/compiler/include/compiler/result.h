// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Compiler result enumeration.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
}  // namespace compiler

#endif  // COMPILER_RESULT_H_INCLUDED
