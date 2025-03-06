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
/// @brief Extendable allocator interface supporting aligned allocations.

#ifndef CARGO_ALLOCATOR_H_INCLUDED
#define CARGO_ALLOCATOR_H_INCLUDED

#include <cargo/attributes.h>

#include <cstddef>
#include <new>
#include <utility>

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief Allocate aligned memory in a cross-platform way.
///
/// @param size Size in bytes to allocate.
/// @param alignment Alignment in bytes of the allocation.
///
/// @note To free memory allocated with this function use `cargo::free`.
///
/// @return A void pointer to the newly allocated memory on success, or
/// `nullptr` otherwise.
void *alloc(size_t size, size_t alignment);

/// @brief Free allocated memory in a cross-platform way.
///
/// @note Should only be used to free memory allocated with `cargo::alloc`.
///
/// @param pointer Pointer to allocated memory.
void free(void *pointer);

/// @brief A simple free store allocator.
///
/// Used by default for classes which require dynamic memory allocations such
/// as `cargo::small_vector` and `cargo::dynamic_array` supplied as a template
/// parameter. If `cargo::mallocator` does not fullfil allocation requirements
/// if can be replaced using a custom implementation of the interface it
/// defined.
///
/// @tparam T Type of object to allocate.
template <class T>
class mallocator {
 public:
  using value_type = T;
  using size_type = size_t;

  /// @brief Allocate an uninitialized contiguous array of `value_type`.
  ///
  /// @param count Number of `value_type` to allocate.
  ///
  /// @return A `value_type` pointer to first element of the array on success,
  /// or `nullptr` otherwise.
  value_type *alloc(size_type count = 1) {
    return static_cast<value_type *>(
        cargo::alloc(sizeof(T) * count, alignof(T)));
  }

  /// @brief Free allocated memory.
  ///
  /// @param pointer Pointer to allocated memory.
  void free(value_type *pointer) { cargo::free(static_cast<void *>(pointer)); }

  /// @brief Allocate and construct an object.
  ///
  /// @tparam Args Constructor argument parameter type pack.
  /// @param args Constructor argument parameter pack.
  ///
  /// @return A constructed `value_type` pointer on success, or `nullptr`
  /// otherwise.
  template <class... Args>
  value_type *create(Args &&...args) {
    value_type *object = static_cast<T *>(cargo::alloc(sizeof(T), alignof(T)));
    new (object) value_type(std::forward<Args>(args)...);
    return object;
  }

  /// @brief Destroy and free a constructed object.
  ///
  /// @param object Pointer to object to be destroyed and freed.
  void destroy(value_type *object) {
    object->~value_type();
    cargo::free(object);
  }
};

/// @brief An allocator which always fails to allocate.
///
/// An allocator which fails to allocate does not seem useful, however when
/// used in combination with `cargo::small_vector` it can ensure that an
/// allocation never occurs when an attempt to grow the vector beyond the small
/// buffer optimization storage. This enables `std::array` like storage
/// guarentees combined with `cargo::small_vector`'s ability to track the
/// number of contained elements.
///
/// @tparam T Type of the object to allocate.
template <class T>
class nullacator {
 public:
  using value_type = T;
  using size_type = size_t;

  /// @brief Allocate an uninitialized contiguous array of `value_type`.
  ///
  /// @return Returns a `nullptr`, always.
  value_type *alloc(size_type = 1) { return nullptr; }

  /// @brief Free allocated memory.
  void free(value_type *) {}

  /// @brief Allocate and construct an object.
  ///
  /// @tparam Args Constructor argument parameter type pack.
  ///
  /// @return Returns a `nullptr`, always.
  template <class... Args>
  value_type *create(Args &&...) {
    return nullptr;
  }

  /// @brief Destroy and free a constructed object.
  void destroy(value_type *) {}
};

/// @brief Delete objects that were constructed in memory from `cargo::alloc`.
///
/// The primary purpose of this `deleter` is to allow `std::unique_ptr` to
/// manage objects in memory allocated by `cargo::alloc`.  I.e. ensure that
/// `cargo::free` is used during deletion.
///
/// @note Should only be used to delete objects that were constructed in memory
/// allocated by `cargo::alloc`.
///
/// Example use:
///
/// ```
///   void *raw = cargo::alloc(sizeof(T), alignof(T));
///   T* object = new (raw) T(...);
///   std::unique_ptr<T, cargo::deleter<T>) ptr(object);
/// ```
///
/// @tparam T Type of the object to delete.
template <class T>
class deleter {
 public:
  /// @brief Destroy and deallocate object.
  ///
  /// @param ptr Pointer to object to be destroyed.
  void operator()(T *ptr) {
    ptr->~T();
    cargo::free(ptr);
  }
};

/// @}
}  // namespace cargo

#endif  // CARGO_ALLOCATOR_H_INCLUDED
