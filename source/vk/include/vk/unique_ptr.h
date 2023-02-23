// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
  deleter(const vk::allocator& allocator) : allocator(allocator) {}

  /// @brief Function call operator to destroy the given object
  ///
  /// @param t Any object created with `allocator` to be destroyed
  void operator()(T t) { allocator.destroy(t); }

  /// @brief Reference to the allocator used for destruction of objects
  const vk::allocator& allocator;
};

/// @brief Alias of `std::unique_ptr` for objects created with `vk::allocator`
///
/// @tparam T Type of any object created with a `vk::allocator`
template <class T>
using unique_ptr = std::unique_ptr<cargo::remove_pointer_t<T>, deleter<T>>;
}  // namespace vk

#endif  // VK_UNIQUE_PTR_H_INCLUDED
