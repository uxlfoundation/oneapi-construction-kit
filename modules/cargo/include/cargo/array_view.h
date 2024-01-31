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
/// @brief A mutable view into an array like object.

#ifndef CARGO_ARRAY_VIEW_H_INCLUDED
#define CARGO_ARRAY_VIEW_H_INCLUDED

#include <cargo/error.h>

#include <algorithm>
#include <cstddef>
#include <type_traits>

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief A mutable view into an array like object.
///
/// Example usage of an array view, all calls to `foo` are equivalent.
///
/// ```cpp
/// void foo(cargo::array_view<int> view) {
///   for (auto &item : view) {
///     // do stuff with item
///   }
/// }
///
/// std::vector<int> v(16, 5);
/// foo(v);
/// foo({v.begin(), v.end()});
/// foo({v.begin(), v.size()});
/// ```
///
/// @tparam T Contained element type.
template <class T>
class array_view {
  template <class U>
  using begin_member_fn = decltype(std::declval<U>().begin());
  template <class U>
  using end_member_fn = decltype(std::declval<U>().end());

 public:
  using value_type = T;
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

  /// @brief Construct default.
  array_view() : Begin(nullptr), End(nullptr) {}

  /// @brief Construct from a pointer and a count.
  ///
  /// @param first Pointer to the first element.
  /// @param count Number of elements in the array.
  array_view(pointer first, size_type count)
      : Begin(first), End(first + count) {}

  /// @brief Construct for an iterator pair.
  ///
  /// @tparam Iterator Type of the iterator.
  /// @param first Iterator at the beginning of the array.
  /// @param last Iterator at the end of the array.
  template <class Iterator>
  array_view(Iterator first, Iterator last)
      :  // MSVC's debug CRT will throw an assertion if you dereference an
         // iterator that is actually beyond the end of the storage. This occurs
         // in two cases: 1) if iterator region is actually empty (first ==
         // last), and 2) when we try and dereference the end iterator to get a
         // pointer. For 1) we do a ternary operation and set Begin & End to
         // nullptr if so, and for 2) we get the iterator of the last actual
         // element in the storage, get the address of that, then add 1 to the
         // result.
        Begin((first == last) ? nullptr : &*first),
        End((first == last) ? nullptr : &*(last - 1) + 1) {}

  /// @brief Construct from a container with begin and end member functions.
  ///
  /// @tparam Container Type of the array like container.
  /// @param container Reference to the container to view.
  template <class Container,
            std::enable_if_t<
#if !defined(_MSC_VER) || \
    (defined(_MSC_VER) && !(_MSC_VER >= 1910 && _MSC_VER < 1921))
                // Disabled for Visual Studio 2017 due to a regression
                // resulting in std::vector<T> failing to compile even though
                // the static_assert's below work as expected, this is likely
                // due to not supporting dependant name binding inside
                // decltype expressions. This issue is fixed in Visual Studio
                // version 2019.
                is_detected<begin_member_fn, Container>::value &&
                is_detected<end_member_fn, Container>::value &&
#endif
                has_value_type_convertible_to<value_type, Container>::value> * =
                nullptr>
  array_view(Container &container)
      : array_view(container.begin(), container.end()) {
#if !defined(_MSC_VER) || \
    (defined(_MSC_VER) && (_MSC_VER >= 1910 && _MSC_VER <= 1914))
    // Work around for is_detected failing to compile in enable_if_t for
    // std::vector<T> (see above) in Visual Studio 2017 likely due to partial
    // support for expression SFINAE.
    static_assert(is_detected<begin_member_fn, Container>::value &&
                      is_detected<end_member_fn, Container>::value,
                  "container.begin() or container.end() was not detected");
#endif
  }

  /// @brief Access element at given position with bounds checking.
  ///
  /// @param pos Position of the element to access.
  ///
  /// @return Returns a reference to the accessed element, or a
  /// `cargo::out_of_range`.
  error_or<reference> at(size_type pos) {
    if (difference_type(pos) >= End - Begin) {
      return cargo::out_of_bounds;
    }
    return Begin[pos];
  }

  /// @brief Access element at given position with bounds checking
  ///
  /// @param pos Position of the element to access.
  ///
  /// @return Returns a const reference to the accessed element.
  error_or<const_reference> at(size_type pos) const {
    if (difference_type(pos) >= End - Begin) {
      return cargo::out_of_bounds;
    }
    return Begin[pos];
  }

  /// @brief Access element at given position.
  ///
  /// @param pos Position of the element to access.
  ///
  /// @return Returns a reference to the accessed element.
  reference operator[](size_type pos) {
    CARGO_ASSERT(difference_type(pos) < End - Begin, "index is out of range");
    return Begin[pos];
  }

  /// @brief Access element at given position.
  ///
  /// @param pos Position of the element to access.
  ///
  /// @return Returns a const reference to the accessed element.
  const_reference operator[](size_type pos) const {
    CARGO_ASSERT(difference_type(pos) < End - Begin, "index is out of range");
    return Begin[pos];
  }

  /// @brief Access first element.
  ///
  /// @return Returns a reference to the first element.
  reference front() {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    return *Begin;
  }

  /// @brief Access first element.
  ///
  /// @return Returns a const reference to the first element.
  const_reference front() const {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    return *Begin;
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
  /// @return Returns a pointer to the array view's data.
  pointer data() {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    return Begin;
  }

  /// @brief Access data.
  ///
  /// @return Returns a const pointer to the array view's data.
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
  /// @return Returns a const iterator pointing to the first element.
  const_iterator begin() const { return Begin; }

  /// @brief Return iterator to beginning.
  ///
  /// @return Returns a const iterator pointing to the first element.
  const_iterator cbegin() const { return Begin; }

  /// @brief Return iterator to end.
  ///
  /// @return Returns an iterator referring to one past the end element.
  iterator end() { return End; }

  /// @brief Return const iterator to end.
  ///
  /// @return Returns a const iterator referring to one past the end element.
  const_iterator end() const { return End; }

  /// @brief Return const iterator to end.
  ///
  /// @return Returns a const iterator referring to one past the end element.
  const_iterator cend() const { return End; }

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

  /// @brief Determine if the array view is empty.
  ///
  /// @return Returns true when empty, false otherwise.
  bool empty() const { return Begin == End; }

  /// @brief Get the size of the array view.
  ///
  /// @return Returns size of the array view.
  size_type size() const { return End - Begin; }

  /// @brief Fill the container with the specified value.
  ///
  /// @param value Value to fill the container with.
  void fill(const_reference value) {
    CARGO_ASSERT(End - Begin, "is empty, invalid access");
    std::fill(Begin, End, value);
  }

  /// @brief Moves the beginning of the array view one element forwards.
  void pop_front() {
    if (!empty()) {
      Begin++;
    }
  }

  /// @brief Moves the end of the array view one element backwards.
  void pop_back() {
    if (!empty()) {
      End--;
    }
  }

 private:
  pointer Begin;
  pointer End;
};

/// @}
}  // namespace cargo

#endif  // CARGO_ARRAY_VIEW_H_INCLUDED
