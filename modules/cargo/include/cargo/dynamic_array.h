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
/// @brief Dynamically allocated fixed size array container.

#ifndef CARGO_DYNAMIC_ARRAY_H_INCLUDED
#define CARGO_DYNAMIC_ARRAY_H_INCLUDED

#include <cargo/allocator.h>
#include <cargo/attributes.h>
#include <cargo/error.h>
#include <cargo/utility.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <new>

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief Dynamically allocated fixed size array container.
///
/// Dynamic array objects can not be copied only moved. Storage for the
/// contained elements of the head array are allocated from the free store.
///
/// @tparam T Element type of the dynamic array.
/// @tparam A The allocator class to use.
template <class T, class A = cargo::mallocator<T>>
class dynamic_array final {
 public:
  using value_type = T;
  using allocator_type = A;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using iterator = value_type *;
  using const_iterator = const value_type *;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  /// @brief Default constructor.
  dynamic_array(allocator_type allocator = allocator_type())
      : Allocator(allocator), Begin(nullptr), End(nullptr) {}

  dynamic_array(const dynamic_array &) = delete;

  /// @brief Move constructor.
  ///
  /// @param other Other dynamic array to steal the guts of.
  dynamic_array(dynamic_array &&other)
      : Allocator(other.Allocator),
        // explicit namespace due to MSVC ADL finding std::exchange
        Begin(cargo::exchange(other.Begin, nullptr)),
        End(cargo::exchange(other.End, nullptr)) {}

  /// @brief Allocate the dynamic array storage and default construct all
  /// elements.
  ///
  /// @param size Number of elements the dynamic array will contain.
  ///
  /// @return Returns `cargo::bad_alloc` on allocation failure, `cargo::success`
  /// otherwise.
  [[nodiscard]] cargo::result alloc(size_type size) {
    if (Begin) {
      clear();
    }
    if (size == 0) {
      return cargo::success;
    }
    Begin = Allocator.alloc(size);
    End = Begin + size;
    if (nullptr == Begin) {
      return cargo::bad_alloc;
    }
    std::for_each(Begin, End, [&](reference item) {
      new (static_cast<void *>(std::addressof(item))) value_type();
    });
    return cargo::success;
  }

  /// @brief Destructor.
  ~dynamic_array() { clear(); }

  /// @brief Move assignment operator.
  ///
  /// @param other Dynamic array object to be moved.
  ///
  /// @return Return reference to this dynamic array.
  dynamic_array &operator=(dynamic_array &&other) {
    if (this == &other) {
      return *this;
    }
    if (Begin) {
      clear();
    }
    Allocator = other.Allocator;
    // explicit namespace due to MSVC ADL finding std::exchange
    Begin = cargo::exchange(other.Begin, nullptr);
    End = cargo::exchange(other.End, nullptr);
    return *this;
  }

  /// @brief Access the element at @p index.
  ///
  /// @param index Index of the element to access.
  ///
  /// @return Return reference to the element at the index.
  error_or<reference> at(size_type index) {
    if (End - Begin <= difference_type(index)) {
      return cargo::out_of_bounds;
    }
    return Begin[index];
  }

  /// @copydoc cargo::dynamic_array::at()
  error_or<const_reference> at(size_type index) const {
    if (End - Begin <= difference_type(index)) {
      return cargo::out_of_bounds;
    }
    return Begin[index];
  }

  /// @brief Index operator to access the element at index.
  ///
  /// @param index Index of the element to access.
  ///
  /// @return Return reference to the element at @p index.
  reference operator[](size_t index) {
    CARGO_ASSERT(Begin && index < size(), "index is out of range");
    return Begin[index];
  }

  /// @brief Index operator to access the element at index.
  ///
  /// @param index Index of the element to access.
  ///
  /// @return Return const reference to the element at @p index.
  const_reference operator[](size_t index) const {
    CARGO_ASSERT(Begin && index < size(), "index is out of range");
    return Begin[index];
  }

  /// @brief Access the first element.
  ///
  /// @return Return reference to the first element.
  reference front() {
    CARGO_ASSERT(Begin, "is empty, invalid access");
    return Begin[0];
  }

  /// @brief Access the first element.
  ///
  /// @return Return const reference to the first element.
  const_reference front() const {
    CARGO_ASSERT(Begin, "is empty, invalid access");
    return Begin[0];
  }

  /// @brief Access the last element.
  ///
  /// @return Return reference to the last element.
  reference back() {
    CARGO_ASSERT(Begin, "is empty, invalid access");
    return *(End - 1);
  }

  /// @brief Access the last element.
  ///
  /// @return Return const reference to the last element.
  const_reference back() const {
    CARGO_ASSERT(Begin, "is empty, invalid access");
    return *(End - 1);
  }

  /// @brief Access the dynamic arrays data.
  ///
  /// @return Return pointer to the beginning of the array.
  pointer data() { return Begin; }

  /// @brief Access the dynamic arrays data.
  ///
  /// @return Return const pointer to the beginning of the array.
  const_pointer data() const { return Begin; }

  /// @brief Access iterator at the beginning.
  ///
  /// @return Return an iterator pointing to the beginning.
  iterator begin() { return Begin; }

  /// @brief Access iterator at the beginning.
  ///
  /// @return Return an iterator pointing to the beginning.
  const_iterator begin() const { return Begin; }

  /// @brief Access iterator at the beginning.
  ///
  /// @return Return an iterator pointing to the beginning.
  const_iterator cbegin() const { return Begin; }

  /// @brief Access reverse iterator at the end.
  ///
  /// @return Returns a reverse iterator pointing at the last element.
  reverse_iterator rbegin() { return reverse_iterator(End); }

  /// @brief Access reverse iterator at the end.
  ///
  /// @return Returns a const reverse iterator pointing at the last element.
  const_reverse_iterator rbegin() const { return const_reverse_iterator(End); }

  /// @brief Access reverse iterator at the end.
  ///
  /// @return Returns a const reverse iterator pointing at the last element.
  const_reverse_iterator crbegin() { return const_reverse_iterator(End); }

  /// @brief Access iterator at the end.
  ///
  /// @return Returns an iterator referring to one past the last element.
  iterator end() { return End; }

  /// @brief Access iterator at the end.
  ///
  /// @return Returns an iterator referring to one past the last element.
  const_iterator end() const { return End; }

  /// @brief Access iterator at the end.
  ///
  /// @return Returns an iterator referring to one past the last element.
  const_iterator cend() const { return End; }

  /// @brief Access reverse iterator at the beginning.
  ///
  /// @return Returns a reverse iterator pointing to the beginning.
  reverse_iterator rend() { return reverse_iterator(Begin); }

  /// @brief Access reverse iterator at the beginning.
  ///
  /// @return Returns a const reverse iterator pointing to the beginning.
  const_reverse_iterator rend() const { return const_reverse_iterator(Begin); }

  /// @brief Access reverse iterator at the beginning.
  ///
  /// @return Returns a const reverse iterator pointing to the beginning.
  const_reverse_iterator crend() { return const_reverse_iterator(Begin); }

  /// @brief Determine if the array is empty.
  ///
  /// @return Return true if the dynamic array has been initialised, false
  /// otherwise.
  bool empty() const { return nullptr == Begin || Begin == End; }

  /// @brief Get the number of elements in the dynamic array.
  ///
  /// @return Return number of elements.
  size_type size() const { return End - Begin; }

  /// @brief Clear the array.
  CARGO_REINITIALIZES void clear() {
    if (Begin) {
      std::for_each(Begin, End, [](reference item) { item.~value_type(); });
      Allocator.free(Begin);
    }
    Begin = nullptr;
    End = nullptr;
  }

 private:
  /// @brief Allocator used for free store memory allocations.
  allocator_type Allocator;
  /// @brief Pointer to beginning of the storage.
  pointer Begin;
  /// @brief Pointer to the end of the storage.
  pointer End;
};

/// @}
}  // namespace cargo

#endif  // CARGO_DYNAMIC_ARRAY_H_INCLUDED
