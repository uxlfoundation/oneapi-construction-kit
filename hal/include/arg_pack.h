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
/// @brief This is utility class to assist a HAL implementation create a packed
/// argument structure that can be passed to a kernel.  It is optional and a
/// HAL implementation is not required to use it.

#ifndef HAL_ARG_PACK_H_INCLUDED
#define HAL_ARG_PACK_H_INCLUDED

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace hal {
/// @addtogroup hal
/// @{

struct hal_arg_t;

namespace util {
/// @addtogroup util
/// @{

struct hal_argpack_t {
  /// @brief Constructor
  ///
  /// @param word_size target processor word size (32 or 64).
  hal_argpack_t(uint32_t word_size) : word_size(word_size) {}

  /// @brief Parse the list of provided argument descriptors and build a packed
  /// argument structure.
  ///
  /// @param args is an array of HAL argument descriptors.
  /// @param num_args the number of argument descriptors provided.
  ///
  /// @return Returns true on success otherwise false.
  bool build(const hal::hal_arg_t *args, uint32_t num_args);

  /// @brief Returns the size in bytes of the packed argument structure.
  ///
  /// @return Return the size in bytes of the packet argument structure.
  const uint64_t size() const { return pack.size(); }

  /// @brief Returns a pointer to the start of the packed argument structure.
  ///
  /// @return Return a pointer to the start of the packed argument structure.
  const void *data() const { return pack.data(); }

  /// @brief Clear the packet argument structure entirely.
  void clear() { pack.clear(); }

 protected:
  /// @brief Append a single argument to the packed argument structure.
  ///
  /// @param arg is the argument descriptor to parse and append.
  ///
  /// @return Returns true on success otherwise false.
  bool add_arg(const hal::hal_arg_t &arg);

  /// @brief Expand the packed argument structure by a number of bytes.
  ///
  /// @param num_bytes is the number of bytes to expand by.
  void expand(size_t num_bytes) { pack.resize(pack.size() + num_bytes); }

  /// @brief Raw packed data of the argument pack.
  std::vector<uint8_t> pack;

  /// @brief Target processor word size.
  const uint32_t word_size;
};

/// @}
}  // namespace util
/// @}
}  // namespace hal

#endif  // HAL_ARG_PACK_H_INCLUDED
