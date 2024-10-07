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
/// @brief Mux's C++ allocation helper.

#ifndef MUX_UTILS_ALLOCATOR_H_INCLUDED
#define MUX_UTILS_ALLOCATOR_H_INCLUDED

#include <mux/mux.h>

#include <new>
#include <utility>

namespace mux {
/// @addtogroup mux_utils
/// @{

/// @brief C++ allocator helper.
///
/// This object is intended to add ease of use functionality to the
/// ::mux_allocator_info_t structure. Upon entering a Mux API function and
/// querying the ::mux_device_t for the ::mux_allocator_info_t immediately
/// construct a local stack instance of this object to access the enhanced
/// functionality. Upon exiting the Mux API function scope this object should
/// be discarded.
class allocator final {
 public:
  /// @brief Constructor.
  ///
  /// @param mux_allocator Reference to a ::mux_allocator_info_t.
  allocator(mux_allocator_info_t &mux_allocator)
      : mux_allocator(mux_allocator) {}

  /// @brief Allocate memory.
  ///
  /// Memory allocated with this function should be freed using
  /// ::mux::allocator::free.
  ///
  /// @tparam Alignment Alignment in bytes of the requested memory.
  /// block.
  /// @param size Size in bytes of the requested memory block.
  ///
  /// @return Return pointer to allocated memory, or null.
  template <size_t Alignment = 1>
  void *alloc(size_t size) {
    return mux_allocator.alloc(mux_allocator.user_data, size, Alignment);
  }

  /// @brief Allocate memory with alignment.
  ///
  /// @param size Size in bytes of the requested memory block.
  /// @param alignment Alignment in bytes of the requested memory block.
  ///
  /// @return Return pointer to allocated memory, or null.
  void *alloc(size_t size, size_t alignment) {
    return mux_allocator.alloc(mux_allocator.user_data, size, alignment);
  }

  /// @brief Allocate an array of uninitialized objects.
  ///
  /// Memory allocated with this function should be freed using
  /// ::mux::allocator::free.
  ///
  /// @tparam T Type of the array elements.
  /// @tparam Alignment Alignment in bytes of the requested array.
  /// @param count Number of elements in the requested array.
  ///
  /// @return Return pointer to allocated array, or nullptr.
  template <typename T, size_t Alignment = alignof(T)>
  T *alloc(size_t count) {
    return static_cast<T *>(mux_allocator.alloc(mux_allocator.user_data,
                                                sizeof(T) * count, Alignment));
  }

  /// @brief Free allocated untyped memory.
  ///
  /// Free memory allocated with ::mux::allocator::alloc.
  ///
  /// @param pointer Pointer allocated using ::mux::allocator::alloc.
  void free(void *pointer) {
    mux_allocator.free(mux_allocator.user_data, pointer);
  }

  /// @brief Allocate and initialized an object.
  ///
  /// Memory allocated with this function should be freed using
  /// ::mux::allocator::destroy.
  ///
  /// @tparam T Type of the object.
  /// @tparam Args Parameter pack of T's constructor arguments types.
  /// @tparam Alignment Alignment in bytes of the requested object.
  /// @param args Parameter pack of T's constructor argument values.
  ///
  /// @return Return an initialized object, or null.
  template <typename T, typename... Args, size_t Alignment = alignof(T)>
  T *create(Args &&...args) {
    void *object =
        mux_allocator.alloc(mux_allocator.user_data, sizeof(T), Alignment);
    if (!object) {
      return nullptr;
    }
    return new (object) T(std::forward<Args>(args)...);
  }

  /// @brief Destroy and free an object.
  ///
  /// @tparam T Type of the object.
  /// @param object The object to destroy.
  template <typename T>
  void destroy(T *object) {
    object->~T();
    mux_allocator.free(mux_allocator.user_data, object);
  }

  /// @brief Get a copy of the underlying allocator info.
  ///
  /// @return Returns a copy of the allocator info.
  mux_allocator_info_t getAllocatorInfo() const { return mux_allocator; }

 private:
  mux_allocator_info_t &mux_allocator;
};

/// @brief Mux allocator for use with `cargo` containers.
///
/// @tparam T Type of object to allocate.
template <class T>
class cargo_allocator {
 public:
  using value_type = T;
  using size_type = size_t;

  /// @brief Construct from a `mux_allocator_info_t`.
  ///
  /// @param allocator_info Allocator information.
  cargo_allocator(mux_allocator_info_t allocator_info)
      : Allocator(allocator_info) {}

  /// @brief Construct from a `mux::allocator`.
  ///
  /// @param allocator Allocator information.
  cargo_allocator(mux::allocator allocator)
      : Allocator(allocator.getAllocatorInfo()) {}

  /// @brief Copy constructor from any specialization.
  ///
  /// @tparam U Allocation type of other cargo_allocator.
  /// @param other Another cargo_allocator to copy from.
  template <class U>
  cargo_allocator(const cargo_allocator<U> &other)
      : Allocator(other.getAllocatorInfo()) {}

  /// @brief Allocate an uninitialized contiguous array of `value_type`.
  ///
  /// @param count Number of `value_type` to allocate.
  ///
  /// @return A `value_type` pointer to first element of the array on
  /// success, `nullptr` otherwise.
  value_type *alloc(size_type count = 1) {
    return static_cast<value_type *>(Allocator.alloc(
        Allocator.user_data, sizeof(value_type) * count, alignof(value_type)));
  }

  /// @brief Free allocated memory.
  ///
  /// @param pointer Pointer to allocated memory.
  void free(value_type *pointer) {
    Allocator.free(Allocator.user_data, static_cast<void *>(pointer));
  }

  /// @brief Allocate and construct an object.
  ///
  /// @tparam Args Constructor argument parameter type pack.
  /// @param args Constructor argument parameter pack.
  ///
  /// @return A constructed `value_type` pointer on success, `nullptr`
  /// otherwise.
  template <class... Args>
  value_type *create(Args &&...args) {
    value_type *object = alloc(1);
    new (static_cast<void *>(object)) value_type(std::forward<Args>(args)...);
    return object;
  }

  /// @brief Destroy and free a constructed object.
  ///
  /// @param object Pointer to object to be destroyed and freed.
  void destroy(value_type *object) {
    object->~value_type();
    free(object);
  }

  /// @brief Get a copy of the underlying allocator info.
  ///
  /// @return Returns a copy of the allocator info.
  mux_allocator_info_t getAllocatorInfo() const { return Allocator; }

 private:
  mux_allocator_info_t Allocator;
};

/// @}
}  // namespace mux

#endif  // MUX_UTILS_ALLOCATOR_H_INCLUDED
