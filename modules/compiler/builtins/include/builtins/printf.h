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
/// @brief Struct and helper functions used on the host for the printf
/// implementation.

#ifndef BUILTINS_PRINTF_H_INCLUDED
#define BUILTINS_PRINTF_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

namespace builtins {
/// @addtogroup builtins
/// @{

namespace printf {
/// @brief List of types relevant to unpacking printf arguments.
///
/// We need to identify float, double and string, and integer types of each
/// width, we don't need to make the distinction between signed and unsigned
/// integer types, printf will still behave correctly.
enum struct type { DOUBLE, FLOAT, LONG, INT, SHORT, CHAR, STRING };

/// @brief Struct containing data representing a single printf call.
///
/// We keep the format string, the types of the arguments as well as the
/// string arguments since OpenCL 1.2 mandates that they are constant, we can
/// extract them at compile time.
struct descriptor final {
  std::string format_string;
  std::vector<type> types;
  std::vector<std::string> strings;

  /// @brief Move constructor
  ///
  /// @param other Object to move
  descriptor(descriptor&& other)
      : format_string(std::move(other.format_string)),
        types(std::move(other.types)),
        strings(std::move(other.strings)) {}

  /// @brief Move assignment operator
  ///
  /// @param other Object to move
  descriptor& operator=(descriptor&& other) {
    format_string = std::move(other.format_string);
    types = std::move(other.types);
    strings = std::move(other.strings);
    return *this;
  }

  /// @brief Deleted copy assignment operator.
  descriptor& operator=(const descriptor&) = delete;

  /// @brief Destructor
  descriptor() {}
};

/// @brief Unpack data from a buffer and print it to the screen.
///
/// @param[in] data Buffer of data to unpack and print.
/// @param[in] max_length Maximum length of the storage buffer `data`.
/// @param[in] printf_calls List of printf calls descriptors that may be found
/// in the buffer.
/// @param[in,out] group offsets Offset into the work group chunk of the buffer
/// to start printing. This function can be called multiple times on the same
/// buffer, to avoid printing duplicate data start from an offset which is set
/// on function return.
void print(uint8_t* data, size_t max_length,
           const std::vector<descriptor>& printf_calls,
           std::vector<uint32_t>& group_offets);
}  // namespace printf

/// @}
}  // namespace builtins

#endif  // BUILTINS_PRINTF_H_INCLUDED
