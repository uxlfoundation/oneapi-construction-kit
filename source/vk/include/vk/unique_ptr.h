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

#ifndef VK_UNIQUE_PTR_H_INCLUDED
#define VK_UNIQUE_PTR_H_INCLUDED

#include <cargo/type_traits.h>
#include <vk/allocator.h>

#include <memory>

namespace vk {
/// @brief Custom deleter for objects created with a `vk::allocator`.
///
/// @tparam T Any object created with a `vk::allocator`
template <class T>
struct deleter {
  /// @brief Constructor
  ///
  /// @param allocator The `vk::allocator` used to destroy the objects
  deleter(const vk::allocator &allocator) : allocator(allocator) {}

  /// @brief Function call operator to destroy the given object
  ///
  /// @param t Any object created with `allocator` to be destroyed
  void operator()(T t) { allocator.destroy(t); }

  /// @brief Reference to the allocator used for destruction of objects
  const vk::allocator &allocator;
};

/// @brief Alias of `std::unique_ptr` for objects created with `vk::allocator`
///
/// @tparam T Type of any object created with a `vk::allocator`
template <class T>
using unique_ptr = std::unique_ptr<std::remove_pointer_t<T>, deleter<T>>;
}  // namespace vk

#endif  // VK_UNIQUE_PTR_H_INCLUDED
