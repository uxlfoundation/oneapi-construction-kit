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
/// @brief Immutable non-owning view of a contiguous sequence of characters.

#ifndef CARGO_STRING_VIEW_H_INCLUDED
#define CARGO_STRING_VIEW_H_INCLUDED

#include <cargo/error.h>
#include <cargo/type_traits.h>

#include <algorithm>
#include <limits>
#include <ostream>
#include <string>

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief An immutable view of a string like object or array.
///
/// Example code:
///
/// ```cpp
/// void print(cargo::string_view sv) {
///   printf("%.*s\n", static_cast<int>(sv.size()), sv.data());
/// }
///
/// print("I'm a string literal.");
/// print(std::string("I'm a std::string");
/// ```
class string_view {
 public:
  using traits_type = std::char_traits<char>;
  using value_type = traits_type::char_type;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;
  using iterator = const_pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  /// @brief Default constructor.
  string_view() : Begin(nullptr), Size(0) {}

  /// @brief Construct view any from `std::string` like object.
  ///
  /// This constructor template enables construction of a `cargo::string_view`
  /// from any type which has `.data()` and `.size()` member functions. Objects
  /// with this API include `std::string`, `std::array<char, N>`,
  /// `std::vector<char>`, `cargo::array_view<const char>`,
  /// `cargo::small_vector`, and `llvm::StringRef`. This allows
  /// `cargo::string_view` to be very permissive about what it is constructed
  /// from without prior knowledge of those types making it a good choice for
  /// string interfaces.
  ///
  /// @note Trailing null terminators are stripped from the
  /// `cargo::string_view` in order to maintain its expected behaviour.
  ///
  /// @note C++17's `std::string_view` does not specify this constructor but
  /// instead added a conversion operator to `std::string` which is less
  /// flexible as it requires changing types we may not have control over.
  ///
  /// @tparam String Type of the `std::string` like object.
  /// @param string Reference to the `std::string` like object.
  template <
      class String,
      std::enable_if_t<is_detected<detail::data_member_fn, String>::value &&
                       is_detected<detail::size_member_fn, String>::value &&
                       has_value_type_convertible_to<value_type, String>::value>
          * = nullptr>
  string_view(const String &string)
      : Begin(string.data()), Size(string.size()) {
    while (Begin + Size - 1 >= Begin && Begin[Size - 1] == '\0') {
      --Size;  // Trailing null terminator found, remove it
    }
  }

  /// @brief Copy constructor.
  ///
  /// @param other View to copy.
  string_view(const string_view &other) = default;

  /// @brief Construct view with first count characters.
  ///
  /// @param string Pointer to first character.
  /// @param count Number of characters.
  string_view(const_pointer string, size_type count)
      : Begin(string), Size(count) {}

  /// @brief Construct view from a null terminated string.
  ///
  /// @param string Either a nullptr or pointer to a null terminated string.
  string_view(const_pointer string)
      : Begin(string),
        Size(string == nullptr ? 0 : traits_type::length(string)) {}

  // TODO: template <Iterator> string_view(Iterator first, Iterator last);

  /// @brief Copy assignment operator.
  ///
  /// @param other View to copy.
  ///
  /// @return Returns a reference to this view.
  string_view &operator=(const string_view &other) = default;

  /// @brief Iterator pointing to the first character.
  ///
  /// @return Returns a const iterator pointing to the first character.
  const_iterator begin() const { return Begin; }

  /// @brief Iterator pointing to the first character.
  ///
  /// @return Returns a const iterator pointing to the first character.
  const_iterator cbegin() const { return Begin; }

  /// @brief Iterator pointing to one past the last character.
  ///
  /// @return Returns a const iterator pointing to one past the last character.
  const_iterator end() const { return Begin + Size; }

  /// @brief Iterator pointing to one past the last character.
  ///
  /// @return Returns a const iterator pointing to one past the last character.
  const_iterator cend() const { return Begin + Size; }

  /// @brief Reverse iterator pointing to one before the first character.
  ///
  /// @return Returns a const reverse iterator pointing to the last character.
  const_reverse_iterator rbegin() const { return reverse_iterator(end()); }

  /// @brief Reverse iterator pointing to one before the last character.
  ///
  /// @return Returns a const reverse iterator pointing to the last character.
  const_reverse_iterator crbegin() const { return reverse_iterator(end()); }

  /// @brief Reverse iterator pointing to one before the first character.
  ///
  /// @return Returns a const reverse iterator pointing to one before the first
  /// character.
  const_reverse_iterator rend() const { return reverse_iterator(begin()); }

  /// @brief Reverse iterator pointing to one before the first character.
  ///
  /// @return Returns a const reverse iterator pointing to one before the first
  /// character.
  const_reverse_iterator crend() const { return reverse_iterator(begin()); }

  /// @brief Access the character at position.
  ///
  /// @param position Index of the character to access.
  ///
  /// @return Returns a const reference to the character at position.
  const_reference operator[](size_type position) const {
    return Begin[position];
  }

  /// @brief Access the character at position with bounds checking.
  ///
  /// @param position Index of the character to access.
  ///
  /// @return Returns a const reference to the character at position or
  /// a `cargo::out_of_bounds` error.
  cargo::error_or<const_reference> at(size_type position) const {
    if (position >= Size) {
      return cargo::out_of_bounds;
    }
    return Begin[position];
  }

  /// @brief Access the first character.
  ///
  /// @return Returns a const reference to the first character.
  const_reference front() const {
    CARGO_ASSERT(Size, "is empty, invalid access");
    return Begin[0];
  }

  /// @brief Access the last character.
  ///
  /// @return Returns a const reference to the last character.
  const_reference back() const {
    CARGO_ASSERT(Size, "is empty, invalid access");
    return Begin[Size - 1];
  }

  /// @brief Access the underlying character array.
  ///
  /// Care should be taken using `cargo::string_view::data()` as it is not
  /// guaranteed to refer to a null terminated character array. *Do not* use
  /// with API's which consume only a `const char *` instead prefer those which
  /// also take the string's size.
  ///
  /// @return Returns a const pointer to the underlying character array.
  const_pointer data() const { return Begin; }

  // TODO: operator llvm::StringRef();

  /// @brief Access the number of characters in the view.
  ///
  /// @return Returns the number of characters in the view.
  size_type size() const { return Size; }

  /// @brief Query the number of characters in the view.
  ///
  /// @return Returns the number of characters in the view.
  size_type length() const { return Size; }

  /// @brief Query the largest possible number of characters in the view.
  ///
  /// @return Returns the largest possible number of characters in the view.
  size_type max_size() const { return npos - 1; }

  /// @brief Check if the view is empty.
  ///
  /// @return Returns true if the view is empty, false otherwise.
  bool empty() const { return 0 == Size; }

  /// @brief Move the begining of the view forward.
  ///
  /// @note The behavior is undefined if `n` is greater than `size()`.
  ///
  /// @param n Number of characters to move the begining of the view forward
  /// by.
  void remove_prefix(size_type n) {
    CARGO_ASSERT(n <= Size, "out of bounds, n is larger than size()");
    Begin += n;
    Size -= n;
  }

  // TODO: void remove_prefix(const_iterator first);

  /// @brief Move the end of the view backward.
  ///
  /// @note The behavior is undefined if `n` is greater than `size()`.
  ///
  /// @param n Number of characters to move the end of the view backward by.
  void remove_suffix(size_type n) {
    CARGO_ASSERT(n <= Size, "out of bounds, n is larger than size()");
    Size -= n;
  }

  // TODO: void remove_suffix(const_iterator last);

  /// @brief Exchange the view with the other.
  ///
  /// @param other View to swap.
  void swap(string_view &other) {
    const string_view temp(other);
    other = *this;
    *this = temp;
  }

  /// @brief Copy the substring to external storage.
  ///
  /// @param dest Character array to copy into.
  /// @param count Number of characters to copy.
  /// @param position Position of the first character.
  ///
  /// @return Returns the number of characters copied, or `cargo::out_of_bounds`
  /// error.
  cargo::error_or<size_type> copy(pointer dest, size_type count,
                                  size_type position = 0) const {
    if (position > Size) {
      return cargo::out_of_bounds;
    }
    count = std::min(Size - position, count);
    traits_type::copy(dest, Begin + position, count);
    return count;
  }

  /// @brief Returns a view of the specified substring.
  ///
  /// @param position Position of the first character.
  /// @param count Number of characters.
  ///
  /// @return Returns a new substring view.
  cargo::error_or<string_view> substr(size_type position = 0,
                                      size_type count = npos) const {
    if (position > Size) {
      return cargo::out_of_bounds;
    }
    return {Begin + position, std::min(Size - position, count)};
  }

  /// @brief Compare this view with another view.
  ///
  /// @param view View to compare.
  ///
  /// @return Returns a negative value if this view is less than the other, zero
  /// if both views are equal, and a positive value if this view is greater than
  /// the other.
  int compare(string_view view) const {
    auto comp = traits_type::compare(begin(), view.begin(),
                                     std::min(size(), view.size()));
    if (0 == comp) {
      comp = size() == view.size() ? 0 : size() < view.size() ? -1 : 1;
    }
    return comp;
  }

  /// @brief Compare a substring of this view with another view.
  ///
  /// @note No bounds checking is performed, it is up to the user to pass valid
  /// values to the fucntion.
  ///
  /// @param position Position of the first character in this view.
  /// @param count Number of characters in this view.
  /// @param view View to compare.
  ///
  /// @return Returns a negative value if this view is less than the other, zero
  /// if both views are equal, and a positive value if this view is greater than
  /// the other.
  int compare(size_type position, size_type count, string_view view) const {
    count = std::min(Size - position, count);
    CARGO_ASSERT((position == 0 && count == 0) || position + count <= Size,
                 "out of bounds, position + count is larger than size()");
    return string_view{Begin + position, count}.compare(view);
  }

  /// @brief Compare a substring of this view with a substring of another view.
  ///
  /// @note No bounds checking is performed, it is up to the user to pass valid
  /// values to the fucntion.
  ///
  /// @param position1 Position of the first character in this view.
  /// @param count1 Number of characters in this view.
  /// @param view View to compare.
  /// @param position2 Position of the first character in the other view.
  /// @param count2 Number of characters in the other view.
  ///
  /// @return Returns a negative value if this view is less than the other, zero
  /// if both views are equal, and a positive value if this view is greater than
  /// the other.
  int compare(size_type position1, size_type count1, string_view view,
              size_type position2, size_type count2) const {
    count1 = std::min(Size - position1, count1);
    CARGO_ASSERT(
        (position1 == 0 && count1 == 0) || position1 + count1 <= size(),
        "out of bounds, position1 + count1 is larger than size()");
    count2 = std::min(view.size() - position2, count2);
    CARGO_ASSERT(
        (position2 == 0 && count2 == 0) || position2 + count2 <= view.size(),
        "out of bounds, position2 + count2 is larger than view.size()");
    return string_view{Begin + position1, count1}.compare(
        string_view{view.begin() + position2, count2});
  }

  /// @brief Compare this view with another null terminated string.
  ///
  /// @param string Null terminated character array to compare.
  ///
  /// @return Returns a negative value if this view is less than the other, zero
  /// if both views are equal, and a positive value if this view is greater than
  /// the other.
  int compare(const_pointer string) const {
    return compare(string_view{string});
  }

  /// @brief Compare a substring of this view with another null terminated
  /// string.
  ///
  /// @note No bounds checking is performed, it is up to the user to pass valid
  /// values to the fucntion.
  ///
  /// @param position Position of the first character in this view.
  /// @param count Number of characters in this view.
  /// @param string Null terminated string to compare.
  ///
  /// @return Returns a negative value if this view is less than the other, zero
  /// if both views are equal, and a positive value if this view is greater than
  /// the other.
  int compare(size_type position, size_type count, const_pointer string) const {
    count = std::min(Size - position, count);
    CARGO_ASSERT((position == 0 && count == 0) || position + count <= size(),
                 "out of bounds, position + count is larger than size()");
    return string_view{Begin + position, count}.compare(string_view{string});
  }

  /// @brief Compare a substring of this view with another string pointer.
  ///
  /// @note No bounds checking is performed, it is up to the user to pass valid
  /// values to the fucntion.
  ///
  /// @param position Position of the first character in this view.
  /// @param count1 Number of characters in this view.
  /// @param string Pointer to first character to compare.
  /// @param count2 Number of characters of `string` to compare.
  ///
  /// @return Returns a negative value if this view is less than the other, zero
  /// if both views are equal, and a positive value if this view is greater than
  /// the other.
  int compare(size_type position, size_type count1, const_pointer string,
              size_type count2) const {
    count1 = std::min(Size - position, count1);
    count2 = std::min(Size, count2);
    CARGO_ASSERT((position == 0 && count1 == 0) || position + count1 <= size(),
                 "out of bounds, position + count1 is larger than size()");
    return string_view{Begin + position, count1}.compare(
        string_view{string, count2});
  }

  /// @brief Check if the string begins with the given prefix.
  ///
  /// @param view The prefix to check for.
  ///
  /// @return Returns true if the string view starts with the prefix, false
  /// otherwise.
  bool starts_with(string_view view) const {
    return size() >= view.size() && compare(0, view.size(), view) == 0;
  }

  /// @brief Check if the string begins with the given prefix.
  ///
  /// @param c The prefix to check for.
  ///
  /// @return Returns true if the string view starts with the prefix, false
  /// otherwise.
  bool starts_with(value_type c) const {
    return starts_with(string_view{std::addressof(c), 1});
  }

  /// @brief Check if the string begins with the given prefix.
  ///
  /// @param string The prefix to check for.
  ///
  /// @return Returns true if the string view starts with the prefix, false
  /// otherwise.
  bool starts_with(const_pointer string) const {
    return starts_with(string_view{string});
  }

  /// @brief Check if the string ends with the given suffix.
  ///
  /// @param view The suffix to check for.
  ///
  /// @return Returns true if the string view ends with the suffix, false
  /// otherwise.
  bool ends_with(string_view view) const {
    return size() >= view.size() &&
           compare(size() - view.size(), npos, view) == 0;
  }

  /// @brief Check if the string ends with the given suffix.
  ///
  /// @param c The suffix to check for.
  ///
  /// @return Returns true if the string view ends with the suffix, false
  /// otherwise.
  bool ends_with(value_type c) const {
    return ends_with(string_view{std::addressof(c), 1});
  }

  /// @brief Check if the string ends with the given suffix.
  ///
  /// @param string The suffix to check for.
  ///
  /// @return Returns true if the string view ends with the suffix, false
  /// otherwise.
  bool ends_with(const_pointer string) const {
    return ends_with(string_view{string});
  }

  /// @brief Find characters in the view.
  ///
  /// @param view View to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns position of the first character found, or
  /// `cargo::string_view::npos` if not found.
  size_type find(string_view view, size_type position = 0) const {
    if (position >= Size || view.size() > Size ||
        view.size() > Size - position) {
      return npos;
    }
    for (auto index = position; index <= Size - view.size(); index++) {
      if (0 == traits_type::compare(Begin + index, view.begin(), view.size())) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find character in the view.
  ///
  /// @param c Character to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns position of the first character found, or
  /// `cargo::string_view::npos` if not found.
  size_type find(value_type c, size_type position = 0) const {
    for (size_type index = position; index < Size; index++) {
      if (traits_type::eq(Begin[index], c)) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find characters in the view.
  ///
  /// @param string Pointer to a string to search for.
  /// @param position Position to start the search.
  /// @param count Number of characters in the substring to search for.
  ///
  /// @return Returns position of the first character found, or
  /// `cargo::string_view::npos` if not found.
  size_type find(const_pointer string, size_type position,
                 size_type count) const {
    return find(string_view(string, count), position);
  }

  /// @brief Find characters in the view.
  ///
  /// @param string Pointer to a null terminated string to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns position of the first character found, or
  /// `cargo::string_view::npos` if not found.
  size_type find(const_pointer string, size_type position = 0) const {
    return find(string_view(string), position);
  }

  /// @brief Find the last equal substring.
  ///
  /// @param view View to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the first character of the found substring, or
  /// `cargo::string_view::npos` if not found.
  size_type rfind(string_view view, size_type position = npos) const {
    if (view.size() > Size) {
      return npos;
    }
    position = std::min(Size - view.size(), position);
    if (view.size() > Size - position) {
      return npos;
    }
    for (difference_type index = position; index >= 0; index--) {
      if (0 == traits_type::compare(Begin + index, view.begin(), view.size())) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find the last equal substring.
  ///
  /// @param c Character to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the first character of the found substring, or
  /// `cargo::string_view::npos` if not found.
  size_type rfind(value_type c, size_type position = npos) const {
    if (Size == 0) {
      return npos;
    }
    position = std::min(Size - 1, position);
    for (difference_type index = position; index >= 0; index--) {
      if (traits_type::eq(Begin[index], c)) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find the last equal substring.
  ///
  /// @param string Pointer to a string to search for.
  /// @param position Position to start the search.
  /// @param count Number of characters in the substring to search for.
  ///
  /// @return Returns the first character of the found substring, or
  /// `cargo::string_view::npos` if not found.
  size_type rfind(const_pointer string, size_type position,
                  size_type count) const {
    return rfind(string_view(string, count), position);
  }

  /// @brief Find the last equal substring.
  ///
  /// @param string Pointer to a null terminated string to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the first character of the found substring, or
  /// `cargo::string_view::npos` if not found.
  size_type rfind(const_pointer string, size_type position = npos) const {
    return rfind(string_view(string), position);
  }

  /// @brief Find the first character equal to any of the given characters.
  ///
  /// @param view View to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the first character found, or
  /// `cargo::string_view::npos` if not found.
  size_type find_first_of(string_view view, size_type position = 0) const {
    for (size_type index = position; index < Size; index++) {
      auto a = Begin[index];
      if (std::any_of(view.begin(), view.end(),
                      [a](value_type b) { return traits_type::eq(a, b); })) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find the first character equal to the given character.
  ///
  /// @param c Character to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the first character found, or
  /// `cargo::string_view::npos` if not found.
  size_type find_first_of(value_type c, size_type position = 0) const {
    for (size_type index = position; index < Size; index++) {
      if (traits_type::eq(Begin[index], c)) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find the first character equal to any of the given characters.
  ///
  /// @param string Pointer to a string to search for.
  /// @param position Position to start the search.
  /// @param count Number of characters to search for.
  ///
  /// @return Returns the position of the first character found, or
  /// `cargo::string_view::npos` if not found.
  size_type find_first_of(const_pointer string, size_type position,
                          size_type count) const {
    return find_first_of(string_view(string, count), position);
  }

  /// @brief Find the first character equal to any of the given characters.
  ///
  /// @param string Pointer to null terminated string to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the first character found, or
  /// `cargo::string_view::npos` if not found.
  size_type find_first_of(const_pointer string, size_type position = 0) const {
    return find_first_of(string_view(string), position);
  }

  /// @brief Find the last character equal to any of the given characters.
  ///
  /// @param view View to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the last occurrence of the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_last_of(string_view view, size_type position = npos) const {
    if (Size == 0) {
      return npos;
    }
    position = std::min(Size - 1, position);
    for (difference_type index = position; index >= 0; index--) {
      auto a = Begin[index];
      if (std::any_of(view.begin(), view.end(),
                      [a](value_type b) { return traits_type::eq(a, b); })) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find the last character equal to any of the given characters.
  ///
  /// @param c Character to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the last occurrence of the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_last_of(value_type c, size_type position = npos) const {
    if (Size == 0) {
      return npos;
    }
    position = std::min(Size - 1, position);
    for (difference_type index = position; index >= 0; index--) {
      if (traits_type::eq(Begin[index], c)) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find the last character equal to any of the given characters.
  ///
  /// @param string Pointer to string to search for.
  /// @param position Position to start the search.
  /// @param count Number of characters to search for.
  ///
  /// @return Returns the position of the last occurrence of the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_last_of(const_pointer string, size_type position,
                         size_type count) const {
    return find_last_of(string_view(string, count), position);
  }

  /// @brief Find the last character equal to any of the given characters.
  ///
  /// @param string Pointer to null terminated string to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the last occurrence of the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_last_of(const_pointer string,
                         size_type position = npos) const {
    return find_last_of(string_view(string), position);
  }

  /// @brief Find the first character not equal to any of the given characters.
  ///
  /// @param view View to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the first character not equal to the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_first_not_of(string_view view, size_type position = 0) const {
    for (auto index = position; index < Size; index++) {
      auto a = Begin[index];
      if (std::none_of(view.begin(), view.end(),
                       [a](value_type b) { return traits_type::eq(a, b); })) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find the first character not equal to any of the given characters.
  ///
  /// @param c Character to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the first character not equal to the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_first_not_of(value_type c, size_type position = 0) const {
    for (size_type index = position; index < Size; index++) {
      if (!traits_type::eq(Begin[index], c)) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find the first character not equal to any of the given characters.
  ///
  /// @param string Pointer to string to search for.
  /// @param position Position to start the search.
  /// @param count Number of characters to search for.
  ///
  /// @return Returns the position of the first character not equal to the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_first_not_of(const_pointer string, size_type position,
                              size_type count) const {
    return find_first_not_of(string_view(string, count), position);
  }

  /// @brief Find the first character not equal to any of the given characters.
  ///
  /// @param string Pointer to null terminated string to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the first character not equal to the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_first_not_of(const_pointer string,
                              size_type position = 0) const {
    return find_first_not_of(string_view(string), position);
  }

  /// @brief Find the last character not equal to any of the given characters.
  ///
  /// @param view View to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the last character not equal to the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_last_not_of(string_view view,
                             size_type position = npos) const {
    if (Size == 0) {
      return npos;
    }
    position = std::min(Size - 1, position);
    for (difference_type index = position; index >= 0; index--) {
      auto a = Begin[index];
      if (std::none_of(view.begin(), view.end(),
                       [a](value_type b) { return traits_type::eq(a, b); })) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find the last character not equal to any of the given characters.
  ///
  /// @param c Character to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the last character not equal to the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_last_not_of(value_type c, size_type position = npos) const {
    if (Size == 0) {
      return npos;
    }
    position = std::min(Size - 1, position);
    for (difference_type index = position; index >= 0; index--) {
      if (!traits_type::eq(Begin[index], c)) {
        return index;
      }
    }
    return npos;
  }

  /// @brief Find the last character not equal to any of the given characters.
  ///
  /// @param string Pointer to string to search for.
  /// @param position Position to start the search.
  /// @param count Number of characters to search for.
  ///
  /// @return Returns the position of the last character not equal to the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_last_not_of(const_pointer string, size_type position,
                             size_type count) const {
    return find_last_not_of(string_view(string, count), position);
  }

  /// @brief Find the last character not equal to any of the given characters.
  ///
  /// @param string Pointer to null terminated string to search for.
  /// @param position Position to start the search.
  ///
  /// @return Returns the position of the last character not equal to the given
  /// characters, or `cargo::string_view::npos` if not found.
  size_type find_last_not_of(const_pointer string,
                             size_type position = npos) const {
    return find_last_not_of(string_view(string), position);
  }

  /// @brief Special value to signify an index is not valid.
  static const size_type npos = std::numeric_limits<size_type>::max();

 private:
  const_pointer Begin;
  size_type Size;
};

/// @brief Test left and right had views are equal.
///
/// @param lhs Left hand view to compare.
/// @param rhs Right hand view to compare.
///
/// @return Returns true is views are equal, false otherwise.
inline bool operator==(string_view lhs, string_view rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  return lhs.compare(rhs) == 0;
}

/// @brief Test left and right had views are not equal.
///
/// @param lhs Left hand view to compare.
/// @param rhs Right hand view to compare.
///
/// @return Returns true is views are not equal, false otherwise.
inline bool operator!=(string_view lhs, string_view rhs) {
  return !(lhs == rhs);
}

/// @brief Test left view is less than the right.
///
/// @param lhs Left hand view to compare.
/// @param rhs Right hand view to compare.
///
/// @return Returns true if the left hand view is less than the right hand view,
/// false otherwise.
inline bool operator<(string_view lhs, string_view rhs) {
  return lhs.compare(rhs) < 0;
}

/// @brief Test left view is less or equal to the right.
///
/// @param lhs Left hand view to compare.
/// @param rhs Right hand view to compare.
///
/// @return Returns true if the left hand view is less than or equal to the
/// right hand view, false otherwise.
inline bool operator<=(string_view lhs, string_view rhs) {
  return lhs.compare(rhs) <= 0;
}

/// @brief Test left view is greater than the right.
///
/// @param lhs Left hand view to compare.
/// @param rhs Right hand view to compare.
///
/// @return Returns true if the left hand view is greater than the right hand
/// view, false otherwise.
inline bool operator>(string_view lhs, string_view rhs) {
  return lhs.compare(rhs) > 0;
}

/// @brief Test left view is greater than or equal than the right.
///
/// @param lhs Left hand view to compare.
/// @param rhs Right hand view to compare.
///
/// @return Returns true if the left hand view is greater than or equal to the
/// right hand view, false otherwise.
inline bool operator>=(string_view lhs, string_view rhs) {
  return lhs.compare(rhs) >= 0;
}

/// @brief Stream the content of the string view to the output stream.
///
/// @param stream The stream to output the string view into.
/// @param view The string view to output.
///
/// @return Returns a reference to the output stream.
inline std::ostream &operator<<(std::ostream &stream,
                                const cargo::string_view &view) {
  stream.write(view.data(), view.size());
  return stream;
}
/// @}

}  // namespace cargo

namespace std {
template <>
struct hash<cargo::string_view> {
  inline size_t operator()(const cargo::string_view &sv) const {
    // FNV-1a hash
    if (sizeof(size_t) == 8) {
      const uint64_t basis{14695981039346656037ULL};
      const uint64_t prime{1099511628211ULL};
      uint64_t hash{basis};
      for (const char &c : sv) {
        hash ^= c;
        hash *= prime;
      }
      return hash;
    } else {
      const uint32_t basis{2166136261U};
      const uint32_t prime{16777619U};
      uint32_t hash{basis};
      for (const char &c : sv) {
        hash ^= c;
        hash *= prime;
      }
      return hash;
    }
  }
};
}  // namespace std

#endif  // CARGO_STRING_VIEW_H_INCLUDED
