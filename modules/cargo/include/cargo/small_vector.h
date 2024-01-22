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
/// @brief Vector with small buffer optimization.

#ifndef CARGO_SMALL_VECTOR_H_INCLUDED
#define CARGO_SMALL_VECTOR_H_INCLUDED

#include <cargo/allocator.h>
#include <cargo/attributes.h>
#include <cargo/error.h>
#include <cargo/memory.h>
#include <cargo/type_traits.h>
#include <cargo/utility.h>
#ifdef CA_CARGO_INSTRUMENTATION_ENABLED
#include <debug/backtrace.h>
#endif

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <memory>

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief Vector with small buffer optimization.
///
/// The `::cargo::small_vector` is a `std::vector` like container with the main
/// difference of embedding storage inside the container. The size of the
/// embedded storage is defined by the user as a template parameter to a value
/// which is expected to avoid a dynamic allocation in the common case. When
/// the capacity of the embedded storage is no longer sufficient it is replaced
/// with dynamically allocated storage and all existing elements are moved into
/// the new storage.
///
/// Member functions which may perform a dynamic allocation returns a
/// `::cargo::bad_alloc` when an allocation failure occurs, otherwise
/// `::cargo::success` is returned. Alternatively member functions which return
/// a value such as an iterator use a `::cargo::error_or<T>` object to return
/// either the value _or_ an appropriate `::cargo::result code` if an error
/// occurred.
///
/// Functions are marked with the `[[nodiscard]]` attribute on compilers which
/// support it which forces the user to always check the return value of member
/// functions which may perform a dynamic allocation which helps to avoid
/// accessing an invalid address. Since it is not possible to return a value
/// from a constructor, no allocations are performed during construction.
///
/// Iterators are invalidated in the event that the `::cargo::small_vector`
/// grows larger that the embedded storage.
///
/// Runtime checks are performed in member functions which access container
/// elements, an always on assertion will be triggered if the container is empty
/// or if an attempt to make an out of bounds access is made.
///
/// @tparam T Type of contained elements.
/// @tparam N Capacity of the embedded storage.
template <class T, size_t N, class A = mallocator<T>>
class small_vector {
  using storage_type = std::aligned_storage_t<sizeof(T), alignof(T)>;

 public:
  using value_type = T;
  using allocator_type = A;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;
  using iterator = value_type *;
  using const_iterator = const value_type *;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  /// @brief Default constructor.
  small_vector(allocator_type allocator = allocator_type())
      : Allocator(allocator), Capacity(N), Begin(getStorage()), End(Begin) {}

  small_vector(const small_vector &) = delete;

  /// @brief Move constructor.
  ///
  /// @param other Other small vector to move from.
  small_vector(small_vector &&other)
      : Allocator(other.Allocator),
        Capacity(other.Capacity),
        Begin(getStorage()),
        End(Begin)
#ifdef CA_CARGO_INSTRUMENTATION_ENABLED
        ,
        MaxCapacity(Capacity),
        MaxSize(other.size())
#endif
  {
    if (other.Capacity <= N) {
      cargo::uninitialized_move(other.Begin, other.End, Begin);
      End = Begin + std::distance(other.Begin, other.End);
      std::for_each(other.Begin, other.End,
                    [](reference item) { item.~value_type(); });
      other.Begin = other.getStorage();
      other.End = other.getStorage();
    } else {
      // explicit namespace due to MSVC ADL finding std::exchange
      Begin = cargo::exchange(other.Begin, other.getStorage());
      End = cargo::exchange(other.End, other.getStorage());
      other.Capacity = N;
    }
  }

  /// @brief Destructor.
  ~small_vector() {
#ifdef CA_CARGO_INSTRUMENTATION_ENABLED
    // Warn of any heap allocations.
    if (MaxCapacity > N) {
      fprintf(stderr,
              "WARNING: Heap allocations increased capacity of "
              "cargo::small_vector to %zu during its lifetime. "
              "This exceeds the SBO stack allocation capacity %zu. "
              "Consider increasing SBO buffer size to %zu.\n",
              MaxCapacity, N, MaxCapacity);
      DEBUG_BACKTRACE;
      // Warn of unused SBO elements in the case that there were
      // no heap allocations.
    } else if (MaxSize != N) {
      fprintf(stderr,
              "WARNING: SBO buffer is of size %zu "
              "but only %zu elements ever used and no heap allocations "
              "made. Consider decreasing SBO buffer size to %zu.\n",
              N, MaxSize, MaxSize);
      DEBUG_BACKTRACE;
    }
#endif
    clear();
    if (N < Capacity) {
      Allocator.free(Begin);
      Begin = getStorage();
      End = Begin;
      Capacity = N;
    }
  }

  small_vector &operator=(const small_vector &) = delete;

  /// @brief Move assign small vector elements.
  ///
  /// @param other Other small vector to move from.
  ///
  /// @return Reference to this small vector.
  small_vector &operator=(small_vector &&other) {
    if (this == &other) {
      return *this;
    }
    std::for_each(Begin, End, [](reference item) { item.~value_type(); });
    if (N < Capacity) {
      Capacity = other.Capacity;
      Allocator.free(Begin);
    }
    Allocator = other.Allocator;
    if (other.Capacity <= N) {
      const size_type Size = other.size();
      cargo::uninitialized_move(other.Begin, other.End, Begin);
      End = Begin + Size;
      std::for_each(other.Begin, other.End,
                    [](reference item) { item.~value_type(); });
      other.Begin = other.getStorage();
      other.End = other.getStorage();
    } else {
      // explicit namespace due to MSVC ADL finding std::exchange
      Begin = ::cargo::exchange(other.Begin, other.getStorage());
      End = ::cargo::exchange(other.End, other.getStorage());
      Capacity = other.Capacity;
      other.Capacity = N;
    }
#ifdef CA_CARGO_INSTRUMENTATION_ENABLED
    MaxCapacity = std::max(MaxCapacity, Capacity);
    MaxSize = std::max(MaxSize, size());
#endif
    return *this;
  }

  /// @brief Assign value to small vector elements.
  ///
  /// @param size Size of content to assign.
  /// @param value Value of content to assign.
  ///
  /// @return Returns `cargo::bad_alloc` on allocation failure, `cargo::success`
  /// otherwise.
  [[nodiscard]] cargo::result assign(size_type size, const_reference value) {
    erase(Begin, End);
    if (auto error = insert(Begin, size, value).error()) {
      return error;
    }
    return cargo::success;
  }

  /// @brief Assign a range to small vector.
  ///
  /// @tparam InputIterator Type of the input iterator.
  /// @param first Beginning of the input range.
  /// @param last End of the input range.
  ///
  /// @return Returns `cargo::bad_alloc` on allocation failure, `cargo::success`
  /// otherwise.
  template <class InputIterator>
  [[nodiscard]] std::enable_if_t<is_input_iterator<InputIterator>::value,
                                 cargo::result>
  assign(InputIterator first, InputIterator last) {
    erase(Begin, End);
    if (auto error = insert(Begin, first, last).error()) {
      return error;
    }
    return cargo::success;
  }

  /// @brief Assign list to small vector.
  ///
  /// @param list List elements to assign.
  ///
  /// @return Returns `cargo::bad_alloc` on allocation failure, `cargo::success`
  /// otherwise.
  [[nodiscard]] cargo::result assign(std::initializer_list<value_type> list) {
    erase(Begin, End);
    if (auto error = insert(Begin, list).error()) {
      return error;
    }
    return cargo::success;
  }

  /// @brief Access the allocator associated with the small vector.
  ///
  /// @return Returns the associated allocator.
  allocator_type get_allocator() const { return Allocator; }

  /// @brief Access element at given index with bounds checking.
  ///
  /// @param index Index of element to access.
  ///
  /// @return Returns a reference to the indexed element, or
  /// `cargo::out_of_bounds`.
  error_or<reference> at(size_type index) {
    if (End - Begin <= difference_type(index)) {
      return cargo::out_of_bounds;
    }
    return Begin[index];
  }

  /// @brief Access element at given index with bounds checking.
  ///
  /// @param index Index of element to access.
  ///
  /// @return Returns a const reference to the indexed element, or
  /// `cargo::out_of_bounds`.
  error_or<const_reference> at(size_type index) const {
    if (End - Begin <= difference_type(index)) {
      return cargo::out_of_bounds;
    }
    return Begin[index];
  }

  /// @brief Access element at given index.
  ///
  /// @param index Index of element to access.
  ///
  /// @return Returns a reference to the indexed element.
  reference operator[](size_type index) {
    CARGO_ASSERT(difference_type(index) < End - Begin, "index is out of range");
    return Begin[index];
  }

  /// @brief Access element at given index.
  ///
  /// @param index Index of element to access.
  ///
  /// @return Returns a const reference to the indexed element.
  const_reference operator[](size_type index) const {
    CARGO_ASSERT(difference_type(index) < End - Begin, "index is out of range");
    return Begin[index];
  }

  /// @brief Access first element.
  ///
  /// @return Returns a reference to the first element.
  reference front() {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    return Begin[0];
  }

  /// @brief Access first element.
  ///
  /// @return Returns a const reference to the first element.
  const_reference front() const {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    return Begin[0];
  }

  /// @brief Access last element.
  ///
  /// @return Returns a reference to the last element.
  reference back() {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    return *(End - 1);
  }

  /// @brief Access last element.
  ///
  /// @return Returns a const reference to the last element.
  const_reference back() const {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    return *(End - 1);
  }

  /// @brief Access data.
  ///
  /// @return Returns a pointer to the small vector's data.
  pointer data() {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    return Begin;
  }

  /// @brief Access data.
  ///
  /// @return Returns a const pointer to the small vector's data.
  const_pointer data() const {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    return Begin;
  }

  /// @brief Return iterator to beginning.
  ///
  /// @return Returns an iterator pointing to the first element.
  iterator begin() { return Begin; }

  /// @brief Return iterator to beginning.
  ///
  /// @return Returns an const iterator pointing to the first element.
  const_iterator begin() const { return Begin; }

  /// @brief Return iterator to beginning.
  ///
  /// @return Returns an const iterator pointing to the first element.
  const_iterator cbegin() const { return Begin; }

  /// @brief Return reverse iterator to end.
  ///
  /// @return Returns a reverse iterator pointing to the last element.
  reverse_iterator rbegin() { return reverse_iterator(End); }

  /// @brief Return reverse iterator to end.
  ///
  /// @return Returns a const reverse iterator pointing to the last element.
  const_reverse_iterator rbegin() const { return const_reverse_iterator(End); }

  /// @brief Return reverse iterator to end.
  ///
  /// @return Returns a const reverse iterator pointing to the last element.
  const_reverse_iterator crbegin() const { return const_reverse_iterator(End); }

  /// @brief Return iterator to end.
  ///
  /// @return Returns an iterator referring to one past the end element.
  iterator end() { return End; }

  /// @brief Return const iterator to end.
  ///
  /// @return Returns an const iterator referring to one past the end element.
  const_iterator end() const { return End; }

  /// @brief Return const iterator to end.
  ///
  /// @return Returns an const iterator referring to one past the end element.
  const_iterator cend() const { return End; }

  /// @brief Return reverse iterator to the beginning.
  ///
  /// @return Returns a reverse iterator referring to one before the first
  /// element.
  reverse_iterator rend() { return reverse_iterator(Begin); }

  /// @brief Return const reverse iterator to the beginning.
  ///
  /// @return Returns a const reverse iterator referring to one before the first
  /// element.
  const_reverse_iterator rend() const { return const_reverse_iterator(Begin); }

  /// @brief Return const reverse iterator to the beginning.
  ///
  /// @return Returns a const reverse iterator referring to one before the first
  /// element.
  const_reverse_iterator crend() const { return const_reverse_iterator(Begin); }

  /// @brief Determine if the small vector is empty.
  ///
  /// @return Returns true when empty, false otherwise.
  bool empty() const { return Begin == End; }

  /// @brief Get the size of the small vector.
  ///
  /// @return Returns size of the small vector.
  size_type size() const { return End - Begin; }

  // TODO: size_type max_size() const;
  // To calculate the theoretical maximum size of the small vector we need to
  // know the size of installed system memory, should cargo be the place we want
  // to implement access to this information?

  /// @brief Request a change in capacity.
  ///
  /// @param size Size of storage to reserve.
  ///
  /// @return Returns `cargo::bad_alloc` on allocation failure, `cargo::success`
  /// otherwise.
  [[nodiscard]] cargo::result reserve(size_type size) {
    if (size <= Capacity) {
      return cargo::success;
    }
    // requirements of value_type.
    pointer begin = Allocator.alloc(size);
    if (!begin) {
      return cargo::bad_alloc;
    }
    move_or_copy(Begin, End, begin);
    if (N < Capacity) {
      Allocator.free(Begin);
    }
    Capacity = size;
#ifdef CA_CARGO_INSTRUMENTATION_ENABLED
    // Keep track of the maximum capacity.
    MaxCapacity = std::max(MaxCapacity, Capacity);
#endif
    const size_type count = End - Begin;
    Begin = begin;
    End = Begin + count;
    return cargo::success;
  }

  /// @brief Get the storage capacity of the small vector.
  ///
  /// @return Returns the number of elements which can be stored.
  size_type capacity() const { return Capacity; }

  /// @brief Attempt to reduce memory consumption.
  void shrink_to_fit() {
    const size_type Size = size();
    if (Size <= N && N < Capacity) {
      cargo::uninitialized_move(Begin, End, getStorage());
      Allocator.free(Begin);
      Capacity = N;
      Begin = getStorage();
      End = Begin + Size;
    }
  }

  /// @brief Clear content.
  CARGO_REINITIALIZES void clear() {
    std::for_each(Begin, End, [](reference item) { item.~value_type(); });
    End = Begin;
  }

  /// @brief Insert a single copied element.
  ///
  /// @param pos Position to insert the element.
  /// @param value Value to copied to the inserted element.
  ///
  /// @note `value_type` must be copy assignable and copy insertable.
  ///
  /// @return Returns iterator at the inserted element, or `cargo::bad_alloc` on
  /// allocation failure.
  template <typename VT = value_type>
  [[nodiscard]] std::enable_if_t<std::is_copy_assignable_v<VT> &&
                                     std::is_copy_constructible_v<VT>,
                                 error_or<iterator>>
  insert(const_iterator pos, const_reference value) {
    CARGO_ASSERT(Begin <= pos && End >= pos, "invalid position");
    const size_type index = pos - Begin;
    if (auto error = extend(1)) {
      return error;
    }
    iterator position = Begin + index;
    std::copy_backward(position, End, End + 1);
    new (std::addressof(*position)) value_type(value);
    setEnd(End + 1);
    return position;
  }

  /// @brief Insert a single moved element.
  ///
  /// @param pos Position to insert the element.
  /// @param value Value to moved to the inserted element.
  ///
  /// @note `value_type` must be move assignable and move insertable.
  ///
  /// @return Returns iterator at the inserted element, or `cargo::bad_alloc` on
  /// allocation failure.
  template <typename VT = value_type>
  [[nodiscard]] std::enable_if_t<std::is_move_assignable_v<VT> &&
                                     std::is_move_constructible_v<VT>,
                                 error_or<iterator>>
  insert(const_iterator pos, value_type &&value) {
    CARGO_ASSERT(Begin <= pos && End >= pos, "invalid position");
    const size_type index = pos - Begin;
    if (auto error = extend(1)) {
      return error;
    }
    iterator position = Begin + index;
    std::move_backward(position, End, End + 1);
    new (std::addressof(*position)) value_type(std::forward<value_type>(value));
    setEnd(End + 1);
    return position;
  }

  /// @brief Insert count copies of the value before position.
  ///
  /// @param pos Position to insert the copied elements.
  /// @param count Number of copied values to insert.
  /// @param value Value to copy into elements.
  ///
  /// @note `value_type` must be copy assignable and copy insertable.
  ///
  /// @return Returns iterator pointing to the first element inserted, or
  /// `cargo::bad_alloc` on allocation failure.
  template <typename VT = value_type>
  [[nodiscard]] std::enable_if_t<std::is_copy_assignable_v<VT> &&
                                     std::is_copy_constructible_v<VT>,
                                 error_or<iterator>>
  insert(const_iterator pos, size_type count, const_reference value) {
    CARGO_ASSERT(Begin <= pos && End >= pos, "invalid position");
    const size_type index = pos - Begin;
    if (auto error = extend(count)) {
      return error;
    }
    iterator position = Begin + index;
    std::copy_backward(position, End, End + count);
    std::uninitialized_fill(position, position + count, value);
    setEnd(End + count);
    return position;
  }

  /// @brief Insert elements from range before position.
  ///
  /// @tparam InputIterator Type of the input iterator.
  /// @param pos Position to insert the range of elements.
  /// @param first First element in the range to insert.
  /// @param last Last element in the range to insert.
  ///
  /// @note `value_type` must be emplace constructable, move assignable and move
  /// insertable.
  ///
  /// @return Returns iterator pointing to the first element inserted, or
  /// `cargo::bad_alloc` on allocation failure.
  template <typename InputIterator, typename VT = value_type>
  [[nodiscard]] std::enable_if_t<is_input_iterator<InputIterator>::value &&
                                     std::is_move_constructible_v<VT> &&
                                     std::is_move_assignable_v<VT>,
                                 error_or<iterator>>
  insert(const_iterator pos, InputIterator first, InputIterator last) {
    CARGO_ASSERT(Begin <= pos && End >= pos, "invalid position");
    const size_type index = pos - Begin;
    const size_type count = std::distance(first, last);
    if (auto error = extend(count)) {
      return error;
    }
    iterator position = Begin + index;
    std::move_backward(position, End, End + count);
    cargo::uninitialized_move(first, last, position);
    setEnd(End + count);
    return position;
  }

  /// @brief Insert elements from initializer list before position.
  ///
  /// @param pos Position to insert iterator list elements.
  /// @param list Iterator list of elements to insert.
  ///
  /// @note `value_type` must be emplace constructable, move assignable and move
  /// insertable.
  ///
  /// @return Returns iterator pointing to the first element inserted, or
  /// `cargo::bad_alloc` on allocation failure.
  template <typename VT = value_type>
  [[nodiscard]] std::enable_if_t<std::is_move_assignable_v<VT> &&
                                     std::is_move_constructible_v<VT>,
                                 error_or<iterator>>
  insert(const_iterator pos, std::initializer_list<VT> list) {
    CARGO_ASSERT(Begin <= pos && End >= pos, "invalid position");
    const size_type index = pos - Begin;
    if (auto error = extend(list.size())) {
      return error;
    }
    iterator position = Begin + index;
    std::move_backward(position, End, End + list.size());
    cargo::uninitialized_move(list.begin(), list.end(), position);
    setEnd(End + list.size());
    return position;
  }

  /// @brief Emplace a new element before position.
  ///
  /// @tparam Args Constructor argument types.
  /// @param pos Position to emplace element.
  /// @param args Constructor arguments values.
  ///
  /// @note `value_type` must be emplace constructable, move assignable and move
  /// insertable.
  ///
  /// @return Returns iterator pointer to the emplace element, or
  /// `cargo::bad_alloc` on allocation failure.
  template <class... Args>
  [[nodiscard]] error_or<iterator> emplace(const_iterator pos, Args &&...args) {
    CARGO_ASSERT(Begin <= pos && End >= pos, "invalid position");
    const size_type index = pos - Begin;
    if (auto error = extend(1)) {
      return error;
    }
    iterator position = Begin + index;
    std::move_backward(position, End, End + 1);
    new (position) value_type(std::forward<Args>(args)...);
    setEnd(End + 1);
    return position;
  }

  /// @brief Erase a single element.
  ///
  /// @param position Position of the element to erase.
  ///
  /// @note `value_type` must be move assignable.
  ///
  /// @return Returns iterator one past the erased element.
  iterator erase(iterator position) {
    CARGO_ASSERT(Begin <= position && End > position, "invalid position");
    std::move(position + 1, End, position);
    End--;
    End->~value_type();
    return position;
  }

  /// @brief Erases elements in the range [first, last).
  ///
  /// If first==last: erasing an empty range is a no-op.
  ///
  /// @param first First element in the range to be erased.
  /// @param last Last element in the range to be erased.
  ///
  /// @note `value_type` must be move assignable.
  ///
  /// @return Returns iterator one past the last erased element.
  iterator erase(iterator first, iterator last) {
    CARGO_ASSERT(Begin <= first && End >= last, "invalid range");
    const iterator old_tail_first = last, old_tail_last = End;
    const size_t tail_size = old_tail_last - old_tail_first;
    const iterator new_tail_first = first,
                   new_tail_last = new_tail_first + tail_size;
    std::move(old_tail_first, old_tail_last, new_tail_first);
    End = new_tail_last;
    std::for_each(new_tail_last, old_tail_last,
                  [](reference item) { item.~value_type(); });
    return new_tail_first;
  }

  /// @brief Add element at the end.
  ///
  /// @param value Value to copy at the end.
  ///
  /// @note `value_type` must be copy insertable.
  ///
  /// @return Returns `cargo::bad_alloc` on allocation failure, `cargo::success`
  /// otherwise.
  template <class ValueType = value_type>
  [[nodiscard]] std::enable_if_t<std::is_copy_constructible_v<ValueType>,
                                 cargo::result>
  push_back(const_reference value) {
    if (auto error = extend(1)) {
      return error;
    }
    new (End) value_type(value);
    setEnd(End + 1);
    return cargo::success;
  }

  /// @brief Add element at the end.
  ///
  /// @param value Value to move to the end.
  ///
  /// @note `value_type` must be move insertable.
  ///
  /// @return Returns `cargo::bad_alloc` on allocation failure, `cargo::success`
  /// otherwise.
  template <class ValueType = value_type>
  [[nodiscard]] std::enable_if_t<std::is_move_constructible_v<ValueType>,
                                 cargo::result>
  push_back(value_type &&value) {
    if (auto error = extend(1)) {
      return error;
    }
    new (End) value_type(std::move(value));
    setEnd(End + 1);
    return cargo::success;
  }

  /// @brief Construct and insert element at the end.
  ///
  /// @tparam Args Variadic constructor argument types.
  /// @param args Variadic constructor argument values.
  ///
  /// @note `value_type` must be emplace constructable and move insertable.
  ///
  /// @return Returns `cargo::bad_alloc` on allocation failure, `cargo::success`
  /// otherwise.
  template <class... Args>
  [[nodiscard]] cargo::result emplace_back(Args &&...args) {
    if (auto error = extend(1)) {
      return error;
    }
    new (End) value_type(std::forward<Args>(args)...);
    setEnd(End + 1);
    return cargo::success;
  }

  /// @brief Remove element at the end.
  void pop_back() {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    End--;
    End->~value_type();
  }

  /// @brief Resize the small vector to contain count elements.
  ///
  /// @param count New size of the small vector.
  ///
  /// @note `value_type` must be move insertable and default insertable.
  ///
  /// @return Returns `cargo::bad_alloc` on allocation failure, `cargo::success`
  /// otherwise.
  [[nodiscard]] cargo::result resize(size_type count) {
    if (auto error = reserve(count)) {
      return error;
    }
    const size_type size = End - Begin;
    if (size > count) {
      std::for_each(Begin + count, End,
                    [](reference item) { item.~value_type(); });
    }
    if (size < count) {
      std::for_each(End, Begin + count, [](reference item) {
        new (std::addressof(item)) value_type();
      });
    }
    End = Begin + count;
    return cargo::success;
  }

  /// @brief Resize the small vector to contain count elements.
  ///
  /// @param count New size of the small vector.
  /// @param value Value to initialize new elements with.
  ///
  /// @note `value_type` must be copy insertable.
  ///
  /// @return Returns `value_type` on allocation failure, `cargo::success`
  /// otherwise.
  [[nodiscard]] cargo::result resize(size_type count, const value_type &value) {
    if (auto error = reserve(count)) {
      return error;
    }
    const size_type size = End - Begin;
    if (size > count) {
      std::for_each(Begin + count, End,
                    [](reference item) { item.~value_type(); });
    }
    if (size < count) {
      std::for_each(End, Begin + count, [&](reference item) {
        new (std::addressof(item)) value_type(value);
      });
    }
    End = Begin + count;
    return cargo::success;
  }

  /// @brief Exchange the content of the container with those of other.
  ///
  /// @note `value_type` must be copy insertable if the capacity of either
  /// small vector is not larger than the embedded storage capacity, otherwise
  /// no move, copy or swap operations will be invoked on individual elements.
  ///
  /// @param other Other small vector for swap with.
  void swap(small_vector &other) {
    auto allocator = Allocator;
    Allocator = other.Allocator;
    Allocator = allocator;
    if (N == Capacity) {
      storage_type storage[N];
      const size_type Size = size();
      iterator begin = reinterpret_cast<iterator>(storage);
      iterator end = begin + Size;
      std::swap_ranges(Begin, End, begin);
      std::swap_ranges(other.Begin, other.End, Begin);
      std::swap_ranges(begin, end, other.Begin);
      const size_type OtherSize = other.size();
      End = Begin + OtherSize;
      other.End = other.Begin + Size;
    } else {
      std::swap(Capacity, other.Capacity);
      std::swap(Begin, other.Begin);
      std::swap(End, other.End);
    }
  }

  /// @brief Create a clone of this small vector.
  ///
  /// Since we don't allow copy construction or assignment clone provides a way
  /// to copy a small vector which allows the user to check for an allocation
  /// failure whilst creating the cloned small vector.
  ///
  /// @note `value_type` must be copy assignable and copy insertable.
  ///
  /// @return Returns a  `cargo::bad_alloc` on allocation.
  cargo::error_or<small_vector> clone() const {
    small_vector other(Allocator);
    if (N < Capacity) {
      if (auto error = other.reserve(Capacity)) {
        return error;
      }
    }
    std::copy(Begin, End, other.Begin);
    other.setEnd(other.Begin + size());
    return other;
  }

 private:
  cargo::result extend(size_type count) {
    count += End - Begin;
    if (Capacity >= count) {
      return cargo::success;
    }
    // TODO: Choose a better expansion metric that takes page size into account.
    return reserve(count * 2);
  }

  // use move if possible, copy otherwise
  template <class Iterator = iterator>
  std::enable_if_t<std::is_move_constructible_v<
      typename std::iterator_traits<Iterator>::value_type>>
  move_or_copy(Iterator first, Iterator last, Iterator dest) {
    cargo::uninitialized_move(first, last, dest);
  }

  template <class Iterator = iterator>
  std::enable_if_t<!std::is_move_constructible_v<
      typename std::iterator_traits<Iterator>::value_type>>
  move_or_copy(Iterator first, Iterator last, Iterator dest) {
    std::uninitialized_copy(first, last, dest);
  }

  /// @brief Access the small vector's embedded storage.
  ///
  /// @return Returns a pointer to the embedded storage.
  pointer getStorage() { return reinterpret_cast<pointer>(Storage); }

  /// Set the value of End.
  ///
  /// @param end Value to set End to.
  /// Useful as a single point to wrap debug code.
  void setEnd(pointer end) {
    End = end;
#ifdef CA_CARGO_INSTRUMENTATION_ENABLED
    MaxSize = std::max(MaxSize, size());
#endif
  }

  /// @brief Allocator used for all free store memory allocations.
  allocator_type Allocator;
  /// @brief Capacity of the small vector.
  size_type Capacity;
  /// @brief Pointer to the beginning of storage.
  pointer Begin;
  /// @brief Pointer to the end of storage.
  pointer End;
  /// @brief Embedded storage, used until capacity is increased.
  storage_type Storage[N];
#ifdef CA_CARGO_INSTRUMENTATION_ENABLED
  /// @brief Maximum capacity of the small_vector during its lifetime.
  ///
  /// Only included in debug builds for the purpose of reporting
  /// when heap allocations exceed the capacity for the SBO. This
  /// should allow better tuning of the size parameter N in the SBO.
  size_type MaxCapacity = N;
  /// @brief Maximum size of the small_vector during its lifetime.
  ///
  /// Only included in debug builds for the purpose of reporting
  /// when SBO elements go unsed in the case there are no heap allocations.
  /// This should allow better tuning of the size parameter N in the SBO.
  size_type MaxSize = 0;
#endif
};

/// @}

/// @brief Determine if two small vector's are equal.
///
/// @tparam T Small vector value type.
/// @tparam SC1 Size of the left small vectors embedded storage.
/// @tparam SC2 Size of the right small vectors embedded stroage.
/// @param left Left hand small vector to be compared.
/// @param right Right hand small vector the be compared.
///
/// @return Returns true if the contents of the containers are equal, false
/// otherwise.
template <class T, size_t SC1, size_t SC2>
bool operator==(const small_vector<T, SC1> &left,
                const small_vector<T, SC2> &right) {
  if (left.size() != right.size()) {
    return false;
  }
  return std::equal(left.begin(), left.end(), right.begin());
}

/// @brief Determine if two small vectors are not equal.
///
/// @tparam T Small vector value type.
/// @tparam SC1 Size of the left small vectors embedded storage.
/// @tparam SC2 Size of the right small vectors embedded stroage.
/// @param left Left hand small vector to be compared.
/// @param right Right hand small vector the be compared.
///
/// @return Returns true if the contents of the containers are not equal, false
/// otherwise.
template <class T, size_t SC1, size_t SC2>
bool operator!=(const small_vector<T, SC1> &left,
                const small_vector<T, SC2> &right) {
  return !(left == right);
}

/// @brief Determine if a small vector is less than another.
///
/// @tparam T Small vector value type.
/// @tparam SC1 Size of the left small vectors embedded storage.
/// @tparam SC2 Size of the right small vectors embedded stroage.
/// @param left Left hand small vector to be compared.
/// @param right Right hand small vector the be compared.
///
/// @return Returns true if the contents of `left` are lexicographically less
/// than the contents of `right`, false otherwise.
template <class T, size_t SC1, size_t SC2>
bool operator<(const small_vector<T, SC1> &left,
               const small_vector<T, SC2> &right) {
  return std::lexicographical_compare(left.begin(), left.end(), right.begin(),
                                      right.end());
}

/// @brief Determine if a small vector is less than or equal to another.
///
/// @tparam T Small vector value type.
/// @tparam SC1 Size of the left small vectors embedded storage.
/// @tparam SC2 Size of the right small vectors embedded stroage.
/// @param left Left hand small vector to be compared.
/// @param right Right hand small vector the be compared.
///
/// @return Returns true if the contents of `left` are lexicographically less
/// than or equal the contents of `right`, false otherwise.
template <class T, size_t SC1, size_t SC2>
bool operator<=(const small_vector<T, SC1> &left,
                const small_vector<T, SC2> &right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(),
      [](const T &a, const T &b) -> bool { return a <= b; });
}

/// @brief Determine if a small vector is greater than another.
///
/// @tparam T Small vector value type.
/// @tparam SC1 Size of the left small vectors embedded storage.
/// @tparam SC2 Size of the right small vectors embedded stroage.
/// @param left Left hand small vector to be compared.
/// @param right Right hand small vector the be compared.
///
/// @return Returns true if the contents of `left` are lexicographically greater
/// than the contents of `right`, false otherwise.
template <class T, size_t SC1, size_t SC2>
bool operator>(const small_vector<T, SC1> &left,
               const small_vector<T, SC2> &right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(),
      [](const T &a, const T &b) -> bool { return a > b; });
}

/// @brief Determine if a small vector is greater than or equal to another.
///
/// @tparam T Small vector value type.
/// @tparam SC1 Size of the left small vectors embedded storage.
/// @tparam SC2 Size of the right small vectors embedded stroage.
/// @param left Left hand small vector to be compared.
/// @param right Right hand small vector the be compared.
///
/// @return Returns true if the contents of `left` are lexicographically greater
/// than or equal the contents of `right`, false otherwise.
template <class T, size_t SC1, size_t SC2>
bool operator>=(const small_vector<T, SC1> &left,
                const small_vector<T, SC2> &right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(),
      [](const T &a, const T &b) -> bool { return a >= b; });
}

template <class T, size_t S>
void swap(cargo::small_vector<T, S> &left, cargo::small_vector<T, S> &right) {
  left.swap(right);
}
}  // namespace cargo

#endif  // CARGO_SMALL_VECTOR_H_INCLUDED
