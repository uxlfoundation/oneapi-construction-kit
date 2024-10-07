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
// Derived from CC0-licensed `tl::expected` library which can be found
// at https://github.com/TartanLlama/expected.  See expected.LICENSE.txt.

/// @file
///
/// @brief An implementation of the proposed std::expected with extensions.

#ifndef CARGO_EXPECTED_H_INCLUDED
#define CARGO_EXPECTED_H_INCLUDED

#include <cargo/detail/expected.h>
#include <cargo/error.h>
#include <cargo/functional.h>

namespace cargo {
/// @addtogroup cargo
/// @{

template <class T, class E>
class expected;

/// @brief Wrapper for storing an unexpected type.
///
/// @tparam E The unexpected type.
template <class E>
class unexpected {
 public:
  static_assert(!std::is_same_v<E, void>, "E must not be void");

  unexpected() = delete;

  /// @brief Copy construct from unexpected value.
  ///
  /// @param e The unexpected value.
  constexpr explicit unexpected(const E &e) : m_val(e) {}

  /// @brief Move construct from the unexpected value.
  ///
  /// @param e The unexpected value.
  constexpr explicit unexpected(E &&e) : m_val(std::move(e)) {}

  /// @brief Access the unexpected value.
  ///
  /// @return Returns a const reference to the unexpected value.
  constexpr const E &value() const & { return m_val; }

  /// @brief Access the unexpected value.
  ///
  /// @return Returns a reference to the unexpected value.
  constexpr E &value() & { return m_val; }

  /// @brief Access the unexpected value.
  ///
  /// @return Returns an r-value reference to the unexpected value.
  constexpr E &&value() && { return std::move(m_val); }

  /// @brief Access the unexpected value.
  ///
  /// @return Returns a const r-value reference to the unexpected value.
  constexpr const E &&value() const && { return std::move(m_val); }

 private:
  E m_val;
};

/// @brief Determine if an unexpected value is equal to another.
///
/// @tparam E The unexpected value type.
/// @param lhs Left hand unexpected to compare.
/// @param rhs Right hand unexpected to compare.
///
/// @return Returns true if values are equal, false otherwise.
template <class E>
constexpr bool operator==(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() == rhs.value();
}

/// @brief Determine if an unexpected value is not equal to another.
///
/// @tparam E The unexpected value type.
/// @param lhs Left hand unexpected to compare.
/// @param rhs Right hand unexpected to compare.
///
/// @return Returns true if values are not equal, false otherwise.
template <class E>
constexpr bool operator!=(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() != rhs.value();
}

/// @brief Determine if an unexpected value is less than another.
///
/// @tparam E The unexpected value type.
/// @param lhs Left hand unexpected to compare.
/// @param rhs Right hand unexpected to compare.
///
/// @return Returns true if `lhs` is less than `rhs`, false otherwise.
template <class E>
constexpr bool operator<(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() < rhs.value();
}

/// @brief Determine if an unexpected value is less than or equal to another.
///
/// @tparam E The unexpected value type.
/// @param lhs Left hand unexpected to compare.
/// @param rhs Right hand unexpected to compare.
///
/// @return Returns true if `lhs` is less than or equal to` rhs`, false
/// otherwise.
template <class E>
constexpr bool operator<=(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() <= rhs.value();
}

/// @brief Determine if an unexpected value is greater than another.
///
/// @tparam E The unexpected value type.
/// @param lhs Left hand unexpected to compare.
/// @param rhs Right hand unexpected to compare.
///
/// @return Returns true if `lhs` is greater than `rhs`, false otherwise.
template <class E>
constexpr bool operator>(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() > rhs.value();
}

/// @brief Determine if an unexpected value is greater than or equal to another.
///
/// @tparam E The unexpected value type.
/// @param lhs Left hand unexpected to compare.
/// @param rhs Right hand unexpected to compare.
///
/// @return Returns true if `lhs` is greater than or equal to` rhs`, false
/// otherwise.
template <class E>
constexpr bool operator>=(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() >= rhs.value();
}

/// @brief Create an unexpected from `e`, deducing the return type.
///
/// @param[in] e Unexpected value to wrap.
///
/// The following two lines of code have the same semantics.
///
/// ```cpp
/// auto e1 = make_unexpected(42);
/// unexpected<int> e2(42);
/// ```
template <class E>
[[nodiscard]] unexpected<std::decay_t<E>> make_unexpected(E &&e) {
  return unexpected<std::decay_t<E>>(std::forward<E>(e));
}

/// @brief A tag type to tell expected to construct the unexpected value.
static constexpr unexpect_t unexpect{};

/// @brief A type which may contain an expected or unexpected value.
///
/// An `expected<T,E>` object is an object that contains the storage for
/// another object and manages the lifetime of this contained object `T`.
/// Alternatively it could contain the storage for another unexpected object
/// `E`. The contained object may not be initialized after the expected object
/// has been initialized, and may not be destroyed before the expected object
/// has been destroyed. The initialization state of the contained object is
/// tracked by the expected object.
///
/// Example usage:
///
/// ```cpp
/// enum class error_t {
///   failure,
///   permission_denied,
///   not_a_directory,
///   insufficient_storage,
/// };
///
/// auto open_file =
///     [](const char *filename) -> cargo::expected<FILE *, error_t> {
///   FILE *file = std::fopen(filename, "w");
///   if (nullptr == file) {
///     switch (errno) {
///       case EACCES:
///         return cargo::make_unexpected(error_t::permission_denied);
///       case ENOTDIR:
///         return cargo::make_unexpected(error_t::not_a_directory);
///       default:
///         return cargo::make_unexpected(error_t::failure);
///     }
///   }
///   return file;
/// };
///
/// auto write_message = [](FILE *file) -> cargo::expected<FILE *, error_t> {
///   const char *message = "hello, expected!\n";
///   std::fwrite(message, 1, std::strlen(message), file);
///   if (errno == ENOMEM) {
///     return cargo::make_unexpected(error_t::insufficient_storage);
///   }
///   return file;
/// };
///
/// auto close_file = [](FILE *file) {
///   std::fclose(file);
/// };
///
/// auto result = open_file("hello_expected.txt")
///                   .and_then(write_message)
///                   .map(close_file);
/// if (!result) {
///   switch (result.error()) {
///     case error_t::failure:
///       printf("failed to open file\n");
///       break;
///     case error_t::permission_denied:
///       printf("permission denied\n");
///       break;
///     case error_t::not_a_directory:
///       printf("not a directory\n");
///       break;
///     case error_t::insufficient_storage:
///       printf("insufficient storage\n");
///       break;
///   }
/// }
/// ```
///
/// @tparam T Type of the expected value.
/// @tparam E Type of the unexpected value.
template <class T, class E>
class expected
    : private detail::expected_move_assign_base<wrap_reference_t<T>, E>,
      private detail::delete_ctor_base<
          detail::is_copy_constructible_or_void_v<wrap_reference_t<T>> &&
              detail::is_copy_constructible_or_void_v<E>,
          std::is_move_constructible_v<wrap_reference_t<T>> &&
              std::is_move_constructible_v<E>>,
      private detail::delete_assign_base<
          std::is_copy_assignable_v<wrap_reference_t<T>> &&
              std::is_copy_assignable_v<E>,
          std::is_move_constructible_v<wrap_reference_t<T>> &&
              std::is_move_constructible_v<E>>,
      private detail::expected_default_ctor_base<wrap_reference_t<T>, E> {
  static_assert(!std::is_same_v<T, std::remove_cv<in_place_t>>,
                "T must not be in_place_t");
  static_assert(!std::is_same_v<T, std::remove_cv<unexpect_t>>,
                "T must not be unexpect_t");
  static_assert(!std::is_same_v<T, std::remove_cv<unexpected<E>>>,
                "T must not be unexpected<E>");
  static_assert(!std::is_reference_v<E>, "E must not be a reference");

  using impl_base = detail::expected_move_assign_base<wrap_reference_t<T>, E>;
  using ctor_base = detail::expected_default_ctor_base<wrap_reference_t<T>, E>;
  using storage_type = wrap_reference_t<T>;

 public:
  using value_type = T;
  using pointer = std::remove_reference_t<T> *;
  using const_pointer = const std::remove_reference_t<T> *;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  /// @brief Default constructor.
  constexpr expected() = default;

  /// @brief Default copy constructor.
  ///
  /// @param rhs Object to copy.
  constexpr expected(const expected &rhs) = default;

  /// @brief Default move constructor.
  ///
  /// @param rhs Object to move.
  constexpr expected(expected &&rhs) = default;

  /// @brief In place value constructor.
  ///
  /// @tparam Args Types of `T`'s constructor arguments.
  /// @param args Forwarded constructor arguments for `T`.
  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<T, Args &&...>> * = nullptr>
  constexpr expected(in_place_t, Args &&...args)
      : impl_base(in_place, std::forward<Args>(args)...),
        ctor_base(detail::default_constructor_tag{}) {}

  /// @brief In place value initializer list value constructor.
  ///
  /// @tparam U Type of initializer list elements.
  /// @tparam Args Types of `T`'s constructor arguments.
  /// @param il Forwarded constructor initializer list for `T`.
  /// @param args Forwarded constructor arguments for `T`.
  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                T, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr expected(in_place_t, std::initializer_list<U> il, Args &&...args)
      : impl_base(in_place, il, std::forward<Args>(args)...),
        ctor_base(detail::default_constructor_tag{}) {}

  /// @brief Unexpected copy constructor.
  ///
  /// @tparam G Type of the unexpected value.
  /// @param e Unexpected value to copy.
  template <class G = E,
            std::enable_if_t<std::is_constructible_v<E, const G &>> * = nullptr,
            std::enable_if_t<!std::is_convertible_v<const G &, E>> * = nullptr>
  explicit constexpr expected(const unexpected<G> &e)
      : impl_base(unexpect, e.value()),
        ctor_base(detail::default_constructor_tag{}) {}

  /// @brief Unexpected copy constructor.
  ///
  /// @tparam G Type of the unexpected value.
  /// @param e Unexpected value to copy.
  template <class G = E,
            std::enable_if_t<std::is_constructible_v<E, const G &>> * = nullptr,
            std::enable_if_t<std::is_convertible_v<const G &, E>> * = nullptr>
  constexpr expected(const unexpected<G> &e)
      : impl_base(unexpect, e.value()),
        ctor_base(detail::default_constructor_tag{}) {}

  /// @brief Unexpected move constructor.
  ///
  /// @tparam G Type of the unexpected value.
  /// @param e Unexpected value to copy.
  template <class G = E,
            std::enable_if_t<std::is_constructible_v<E, G &&>> * = nullptr,
            std::enable_if_t<!std::is_convertible_v<G &&, E>> * = nullptr>
  explicit constexpr expected(unexpected<G> &&e) noexcept(
      std::is_nothrow_constructible_v<E, G &&>)
      : impl_base(unexpect, std::move(e.value())),
        ctor_base(detail::default_constructor_tag{}) {}

  /// @brief Unexpected move constructor.
  ///
  /// @tparam G Type of the unexpected value.
  /// @param e Unexpected value to copy.
  template <class G = E,
            std::enable_if_t<std::is_constructible_v<E, G &&>> * = nullptr,
            std::enable_if_t<std::is_convertible_v<G &&, E>> * = nullptr>
  constexpr expected(unexpected<G> &&e) noexcept(
      std::is_nothrow_constructible_v<E, G &&>)
      : impl_base(unexpect, std::move(e.value())),
        ctor_base(detail::default_constructor_tag{}) {}

  /// @brief In place unexpected constructor.
  ///
  /// @tparam Args Types of `E`'s constructor arguments.
  /// @param args Forwarded constructor arguments for `E`.
  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<E, Args &&...>> * = nullptr>
  constexpr explicit expected(unexpect_t, Args &&...args)
      : impl_base(unexpect, std::forward<Args>(args)...),
        ctor_base(detail::default_constructor_tag{}) {}

  /// @brief In place value initializer list unexpected constructor.
  ///
  /// @tparam U Type of initializer list elements.
  /// @tparam Args Types of `E`'s constructor arguments.
  /// @param il Forwarded constructor initializer list for `E`.
  /// @param args Forwarded constructor arguments for `E`.
  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                E, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr explicit expected(unexpect_t, std::initializer_list<U> il,
                              Args &&...args)
      : impl_base(unexpect, il, std::forward<Args>(args)...),
        ctor_base(detail::default_constructor_tag{}) {}

  /// @brief Copy constructor.
  ///
  /// @tparam U Type of `rhs`'s expected value.
  /// @tparam G Type of `rhs'`s unexpected value.
  /// @param rhs Expected object to copy.
  template <
      class U, class G,
      std::enable_if_t<!(std::is_convertible_v<const U &, T> &&
                         std::is_convertible_v<const G &, E>)> * = nullptr,
      detail::expected_enable_from_other<T, E, U, G, const U &, const G &> * =
          nullptr>
  explicit constexpr expected(const expected<U, G> &rhs)
      : ctor_base(detail::default_constructor_tag{}) {
    if (rhs.has_value()) {
      this->construct(*rhs);
    } else {
      this->construct_error(rhs.error());
    }
  }

  /// @brief Copy constructor.
  ///
  /// @tparam U Type of `rhs`'s expected value.
  /// @tparam G Type of `rhs'`s unexpected value.
  /// @param rhs Expected object to copy.
  template <class U, class G,
            std::enable_if_t<(std::is_convertible_v<const U &, T> &&
                              std::is_convertible_v<const G &, E>)> * = nullptr,
            detail::expected_enable_from_other<T, E, U, G, const U &, const G &>
                * = nullptr>
  constexpr expected(const expected<U, G> &rhs)
      : ctor_base(detail::default_constructor_tag{}) {
    if (rhs.has_value()) {
      this->construct(*rhs);
    } else {
      this->construct_error(rhs.error());
    }
  }

  /// @brief Move constructor.
  ///
  /// @tparam U Type of `rhs`'s expected value.
  /// @tparam G Type of `rhs'`s unexpected value.
  /// @param rhs Expected object to move.
  template <
      class U, class G,
      std::enable_if_t<!(std::is_convertible_v<U &&, T> &&
                         std::is_convertible_v<G &&, E>)> * = nullptr,
      detail::expected_enable_from_other<T, E, U, G, U &&, G &&> * = nullptr>
  explicit constexpr expected(expected<U, G> &&rhs)
      : ctor_base(detail::default_constructor_tag{}) {
    if (rhs.has_value()) {
      this->construct(std::move(*rhs));
    } else {
      this->construct_error(std::move(rhs.error()));
    }
  }

  /// @brief Move constructor.
  ///
  /// @tparam U Type of `rhs`'s expected value.
  /// @tparam G Type of `rhs'`s unexpected value.
  /// @param rhs Expected object to move.
  template <
      class U, class G,
      std::enable_if_t<(std::is_convertible_v<U &&, T> &&
                        std::is_convertible_v<G &&, E>)> * = nullptr,
      detail::expected_enable_from_other<T, E, U, G, U &&, G &&> * = nullptr>
  constexpr expected(expected<U, G> &&rhs)
      : ctor_base(detail::default_constructor_tag{}) {
    if (rhs.has_value()) {
      this->construct(std::move(*rhs));
    } else {
      this->construct_error(std::move(rhs.error()));
    }
  }

  /// @brief Expected value move constructor.
  ///
  /// @tparam U Type of expected value.
  /// @param v Expected value to move.
  template <class U = T,
            std::enable_if_t<!std::is_convertible_v<U &&, T>> * = nullptr,
            detail::expected_enable_forward_value<T, E, U> * = nullptr>
  explicit constexpr expected(U &&v) : expected(in_place, std::forward<U>(v)) {}

  // NOLINTBEGIN(modernize-type-traits)
  /// @brief Expected value move constructor.
  ///
  /// @tparam U Type of expected value.
  /// @param v Expected value to move.
  template <class U = T,
            std::enable_if_t<std::is_convertible_v<U &&, T>> * = nullptr,
            detail::expected_enable_forward_value<T, E, U> * = nullptr>
  // NOLINTEND(modernize-type-traits)
  constexpr expected(U &&v) : expected(in_place, std::forward<U>(v)) {}

  /// @brief Default copy assignment operator.
  ///
  /// @param rhs Object to copy.
  expected &operator=(const expected &rhs) = default;

  /// @brief Default move assignment operator.
  ///
  /// @param rhs Object to move.
  expected &operator=(expected &&rhs) = default;

  // NOLINTBEGIN(modernize-type-traits)
  /// @brief Expected value move assignment operator.
  ///
  /// @tparam U Type of expected value.
  /// @param v Expected value to move.
  template <
      class U = T, class G = T,
      std::enable_if_t<std::is_nothrow_constructible_v<T, U &&>> * = nullptr,
      std::enable_if_t<!std::is_void_v<G>> * = nullptr,
      std::enable_if_t<
          (!std::is_same_v<expected<T, E>, std::decay_t<U>> &&
           !(std::is_scalar_v<T> && std::is_same_v<T, std::decay_t<U>>) &&
           std::is_constructible_v<T, U> && std::is_assignable_v<G &, U> &&
           std::is_nothrow_move_constructible_v<E>)> * = nullptr>
  // NOLINTEND(modernize-type-traits)
  expected &operator=(U &&v) {
    if (has_value()) {
      val() = std::forward<U>(v);
    } else {
      err().~unexpected<E>();
      new (valptr()) T(std::forward<U>(v));
      this->m_has_val = true;
    }
    return *this;
  }

  // NOLINTBEGIN(modernize-type-traits)
  /// @brief Expected value move assignment operator.
  ///
  /// @tparam U Type of expected value.
  /// @param v Expected value to move.
  template <
      class U = T, class G = T,
      std::enable_if_t<!std::is_nothrow_constructible_v<T, U &&>> * = nullptr,
      std::enable_if_t<!std::is_void_v<U>> * = nullptr,
      std::enable_if_t<
          (!std::is_same_v<expected<T, E>, std::decay_t<U>> &&
           !(std::is_scalar_v<T> && std::is_same_v<T, std::decay_t<U>>) &&
           std::is_constructible_v<T, U> && std::is_assignable_v<G &, U> &&
           std::is_nothrow_move_constructible_v<E>)> * = nullptr>
  // NOLINTEND(modernize-type-traits)
  expected &operator=(U &&v) {
    if (has_value()) {
      val() = std::forward<U>(v);
    } else {
      err().~unexpected<E>();
      new (valptr()) T(std::forward<U>(v));
      this->m_has_val = true;
    }
    return *this;
  }

  /// @brief Unexpected value copy assignment operator.
  ///
  /// @tparam G Type of unexpected value.
  /// @param rhs Expected value to copy.
  template <class G = E,
            std::enable_if_t<std::is_nothrow_copy_constructible_v<G> &&
                             std::is_assignable_v<G &, G>> * = nullptr>
  expected &operator=(const unexpected<G> &rhs) {
    if (!has_value()) {
      err() = rhs;
    } else {
      val().~storage_type();
      new (errptr()) unexpected<E>(rhs);
      this->m_has_val = false;
    }
    return *this;
  }

  /// @brief Unexpected value move assignment operator.
  ///
  /// @tparam G Type of unexpected value.
  /// @param rhs Expected value to move.
  template <class G = E,
            std::enable_if_t<std::is_nothrow_move_constructible_v<G> &&
                             std::is_move_assignable_v<G>> * = nullptr>
  expected &operator=(unexpected<G> &&rhs) noexcept {
    if (!has_value()) {
      err() = std::move(rhs);
    } else {
      val().~storage_type();
      new (errptr()) unexpected<E>(std::move(rhs));
      this->m_has_val = false;
    }
    return *this;
  }

  /// @brief Invoke a callable returning an expected on the stored object, if
  /// there is one.
  ///
  /// @note Requires that `invoke(std::forward<F>(f), value())` returns a
  /// `expected<U,E>` for some type `U`.
  ///
  /// @tparam F Type of the callable object.
  /// @param[in] f Callable to invoke on the stored object.
  ///
  /// @return Returns the result of the callable.
  /// @retval If `has_value()` is true, the value returned from
  /// `invoke(std::forward<F>(f), value())` is returned.
  /// @retval If `has_value()` is false, the returned value is empty.
  template <class F>
  constexpr auto and_then(F &&f) & {
    return and_then_impl(*this, std::forward<F>(f));
  }

  /// @brief Invoke a callable returning an expected on the stored object, if
  /// there is one.
  ///
  /// @note Requires that `invoke(std::forward<F>(f), value())` returns a
  /// `expected<U,E>` for some type `U`.
  ///
  /// @tparam F Type of the callable object.
  /// @param[in] f Callable to invoke on the stored object.
  ///
  /// @return Returns the result of the callable.
  /// @retval If `has_value()` is true, the value returned from
  /// `invoke(std::forward<F>(f), value())` is returned.
  /// @retval If `has_value()` is false, the returned value is empty.
  template <class F>
  constexpr auto and_then(F &&f) && {
    return and_then_impl(std::move(*this), std::forward<F>(f));
  }

  /// @brief Invoke a callable returning an expected on the stored object, if
  /// there is one.
  ///
  /// @note Requires that `invoke(std::forward<F>(f), value())` returns a
  /// `expected<U,E>` for some type `U`.
  ///
  /// @tparam F Type of the callable object.
  /// @param[in] f Callable to invoke on the stored object.
  ///
  /// @return Returns the result of the callable.
  /// @retval If `has_value()` is true, the value returned from
  /// `invoke(std::forward<F>(f), value())` is returned.
  /// @retval If `has_value()` is false, the returned value is empty.
  template <class F>
  constexpr auto and_then(F &&f) const & {
    return and_then_impl(*this, std::forward<F>(f));
  }

  /// @brief Invoke a callable returning an expected on the stored object, if
  /// there is one.
  ///
  /// @note Requires that `invoke(std::forward<F>(f), value())` returns a
  /// `expected<U,E>` for some type `U`.
  ///
  /// @tparam F Type of the callable object.
  /// @param[in] f Callable to invoke on the stored object.
  ///
  /// @return Returns the result of the callable.
  /// @retval If `has_value()` is true, the value returned from
  /// `invoke(std::forward<F>(f), value())` is returned.
  /// @retval If `has_value()` is false, the returned value is empty.
  template <class F>
  constexpr auto and_then(F &&f) const && {
    return and_then_impl(std::move(*this), std::forward<F>(f));
  }

  /// @brief Invoke a callable on the stored object, if there is one.
  ///
  /// @tparam F Type of the callable object.
  /// @param[in] f Callable to invoke on the value.
  ///
  /// @return Returns the result of the callable wrapped in a
  /// `expected<U,E>` where `U` is the type returned from the callable.
  /// @retval If `U` is not `void`, returns an `expected<U,E>`.
  /// @retval If `U` is `void`, returns an `expected<monostate,E>`.
  /// @retval If `has_value()` is true, an `expected<U,E>` is constructed from
  /// the return value of `invoke(std::forward<F>(f), value())` and
  /// returned.
  /// @retval If `has_value()` is false, the result is `*this`.
  template <class F>
  constexpr auto map(F &&f) & {
    return expected_map_impl(*this, std::forward<F>(f));
  }

  /// @brief Invoke a callable on the stored object, if there is one.
  ///
  /// @tparam F Type of the callable object.
  /// @param[in] f Callable to invoke on the value.
  ///
  /// @return Returns the result of the callable wrapped in a
  /// `expected<U,E>` where `U` is the type returned from the callable.
  /// @retval If `U` is not `void`, returns an `expected<U,E>`.
  /// @retval If `U` is `void`, returns an `expected<monostate,E>`.
  /// @retval If `has_value()` is true, an `expected<U,E>` is constructed from
  /// the return value of `invoke(std::forward<F>(f), value())` and
  /// returned.
  /// @retval If `has_value()` is false, the result is `*this`.
  template <class F>
  constexpr auto map(F &&f) && {
    return expected_map_impl(std::move(*this), std::forward<F>(f));
  }

  /// @brief Invoke a callable on the stored object, if there is one.
  ///
  /// @tparam F Type of the callable object.
  /// @param[in] f Callable to invoke on the value.
  ///
  /// @return Returns the result of the callable wrapped in a
  /// `expected<U,E>` where `U` is the type returned from the callable.
  /// @retval If `U` is not `void`, returns an `expected<U,E>`.
  /// @retval If `U` is `void`, returns an `expected<monostate,E>`.
  /// @retval If `has_value()` is true, an `expected<U,E>` is constructed from
  /// the return value of `invoke(std::forward<F>(f), value())` and
  /// returned.
  /// @retval If `has_value()` is false, the result is `*this`.
  template <class F>
  constexpr auto map(F &&f) const & {
    return expected_map_impl(*this, std::forward<F>(f));
  }

  /// @brief Invoke a callable on the stored object, if there is one.
  ///
  /// @tparam F Type of the callable object.
  /// @param[in] f Callable to invoke on the value.
  ///
  /// @return Returns the result of the callable wrapped in a
  /// `expected<U,E>` where `U` is the type returned from the callable.
  /// @retval If `U` is not `void`, returns an `expected<U,E>`.
  /// @retval If `U` is `void`, returns an `expected<monostate,E>`.
  /// @retval If `has_value()` is false, the result is `*this`.
  /// @retval If `has_value()` is true, an `expected<U,E>` is constructed from
  /// the return value of `invoke(std::forward<F>(f), value())` and
  /// returned.
  template <class F>
  constexpr auto map(F &&f) const && {
    return expected_map_impl(std::move(*this), std::forward<F>(f));
  }

  /// @brief Invoke a callable on the stored unexpected object, if there is
  /// one.
  ///
  /// @tparam F Type of the callable object.
  /// @param f Callable to invoke on the unexpected value.
  ///
  /// @return Returns an expected, where `U` is the result type of
  /// `invoke(std::forward<F>(f), value())`.
  /// @retval If `U` is `void`, return an `expected<T,monostate>`.
  /// @retval If `U` is not `void`, returns `expected<T,U>`.
  /// @retval If `has_value()` is true, return `*this`.
  /// @retval If `has_value()` is false, return a newly constructed
  /// `expected<T,U>` from
  /// `make_unexpected(invoke(std::forward<F>(f), value()))`.
  template <class F>
  constexpr auto map_error(F &&f) & {
    return map_error_impl(*this, std::forward<F>(f));
  }

  /// @brief Invoke a callable on the stored unexpected object, if there is
  /// one.
  ///
  /// @tparam F Type of the callable object.
  /// @param f Callable to invoke on the unexpected value.
  ///
  /// @return Returns an expected, where `U` is the result type of
  /// `invoke(std::forward<F>(f), value())`.
  /// @retval If `U` is `void`, return an `expected<T,monostate>`.
  /// @retval If `U` is not `void`, returns `expected<T,U>`.
  /// @retval If `has_value()` is true, return `*this`.
  /// @retval If `has_value()` is false, return a newly constructed
  /// `expected<T,U>` from
  /// `make_unexpected(invoke(std::forward<F>(f), value()))`.
  template <class F>
  constexpr auto map_error(F &&f) && {
    return map_error_impl(std::move(*this), std::forward<F>(f));
  }

  /// @brief Invoke a callable on the stored unexpected object, if there is
  /// one.
  ///
  /// @tparam F Type of the callable object.
  /// @param f Callable to invoke on the unexpected value.
  ///
  /// @return Returns an expected, where `U` is the result type of
  /// `invoke(std::forward<F>(f), value())`.
  /// @retval If `U` is `void`, return an `expected<T,monostate>`.
  /// @retval If `U` is not `void`, returns `expected<T,U>`.
  /// @retval If `has_value()` is true, return `*this`.
  /// @retval If `has_value()` is false, return a newly constructed
  /// `expected<T,U>` from
  /// `make_unexpected(invoke(std::forward<F>(f), value()))`.
  template <class F>
  constexpr auto map_error(F &&f) const & {
    return map_error_impl(*this, std::forward<F>(f));
  }

  /// @brief Invoke a callable on the stored unexpected object, if there is
  /// one.
  ///
  /// @tparam F Type of the callable object.
  /// @param f Callable to invoke on the unexpected value.
  ///
  /// @return Returns an expected, where `U` is the result type of
  /// `invoke(std::forward<F>(f), value())`.
  /// @retval If `U` is `void`, return an `expected<T,monostate>`.
  /// @retval If `U` is not `void`, returns `expected<T,U>`.
  /// @retval If `has_value()` is true, return `*this`.
  /// @retval If `has_value()` is false, return a newly constructed
  /// `expected<T,U>` from
  /// `make_unexpected(invoke(std::forward<F>(f), value()))`.
  template <class F>
  constexpr auto map_error(F &&f) const && {
    return map_error_impl(std::move(*this), std::forward<F>(f));
  }

  /// @brief Invoke a callable when in an unexpected state.
  ///
  /// @note Requires that callable `F` is invokable with type `E` and
  /// `invoke_result_t<F>` must be convertible to `expected<T,E>`.
  ///
  /// @tparam F Type of the callable object.
  /// @param f Callable to invoke when in the unexpected state.
  ///
  /// @return Returns an expected, invoking callable on the unexpected object.
  /// @retval If `has_value()` is true, returns `*this`.
  /// @retval If `has_value()` is false and `f` returns `void`, invoke `f` and
  /// return `expected<T,monostate>`.
  /// @retval If `has_value()` is false and `f` does not return `void`, return
  /// `std::forward<F>(f)(error())`.
  template <class F>
  expected constexpr or_else(F &&f) & {
    return or_else_impl(*this, std::forward<F>(f));
  }

  /// @brief Invoke a callable when in an unexpected state.
  ///
  /// @note Requires that callable `F` is invokable with type `E` and
  /// `invoke_result_t<F>` must be convertible to `expected<T,E>`.
  ///
  /// @tparam F Type of the callable object.
  /// @param f Callable to invoke when in the unexpected state.
  ///
  /// @return Returns an expected, invoking callable on the unexpected object.
  /// @retval If `has_value()` is true, returns `*this`.
  /// @retval If `has_value()` is false and `f` returns `void`, invoke `f` and
  /// return `expected<T,monostate>`.
  /// @retval If `has_value()` is false and `f` does not return `void`, return
  /// `std::forward<F>(f)(error())`.
  template <class F>
  expected constexpr or_else(F &&f) && {
    return or_else_impl(std::move(*this), std::forward<F>(f));
  }

  /// @brief Invoke a callable when in an unexpected state.
  ///
  /// @note Requires that callable `F` is invokable with type `E` and
  /// `invoke_result_t<F>` must be convertible to `expected<T,E>`.
  ///
  /// @tparam F Type of the callable object.
  /// @param f Callable to invoke when in the unexpected state.
  ///
  /// @return Returns an expected, invoking callable on the unexpected object.
  /// @retval If `has_value()` is true, returns `*this`.
  /// @retval If `has_value()` is false and `f` returns `void`, invoke `f` and
  /// return `expected<T,monostate>`.
  /// @retval If `has_value()` is false and `f` does not return `void`, return
  /// `std::forward<F>(f)(error())`.
  template <class F>
  expected constexpr or_else(F &&f) const & {
    return or_else_impl(*this, std::forward<F>(f));
  }

  /// @brief Invoke a callable when in an unexpected state.
  ///
  /// @note Requires that callable `F` is invokable with type `E` and
  /// `invoke_result_t<F>` must be convertible to `expected<T,E>`.
  ///
  /// @tparam F Type of the callable object.
  /// @param f Callable to invoke when in the unexpected state.
  ///
  /// @return Returns an expected, invoking callable on the unexpected object.
  /// @retval If `has_value()` is true, returns `*this`.
  /// @retval If `has_value()` is false and `f` returns `void`, invoke `f` and
  /// return `expected<T,monostate>`.
  /// @retval If `has_value()` is false and `f` does not return `void`, return
  /// `std::forward<F>(f)(error())`.
  template <class F>
  expected constexpr or_else(F &&f) const && {
    return or_else_impl(std::move(*this), std::forward<F>(f));
  }

  /// @brief Emplace construct an expected value.
  ///
  /// @tparam Args Type of `T`'s constructor arguments.
  /// @param args Forwarded constructor arguments for `T`.
  template <class... Args,
            std::enable_if_t<std::is_nothrow_constructible_v<T, Args &&...>> * =
                nullptr>
  void emplace(Args &&...args) {
    if (has_value()) {
      val() = T(std::forward<Args>(args)...);
    } else {
      err().~unexpected<E>();
      new (valptr()) T(std::forward<Args>(args)...);
      this->m_has_val = true;
    }
  }

  /// @brief Emplace construct an expected value.
  ///
  /// @tparam Args Type of `T`'s constructor arguments.
  /// @param args Forwarded constructor arguments for `T`.
  template <class... Args,
            std::enable_if_t<!std::is_nothrow_constructible_v<T, Args &&...>>
                * = nullptr>
  void emplace(Args &&...args) {
    if (has_value()) {
      val() = T(std::forward<Args>(args)...);
    } else {
      err().~unexpected<E>();
      new (valptr()) T(std::forward<Args>(args)...);
      this->m_has_val = true;
    }
  }

  /// @brief Emplace initializer list construct an expected value.
  ///
  /// @tparam U Type of initializer list elements.
  /// @tparam Args Types of `E`'s constructor arguments.
  /// @param il Forwarded constructor initializer list for `E`.
  /// @param args Forwarded constructor arguments for `E`.
  template <class U, class... Args,
            std::enable_if_t<std::is_nothrow_constructible_v<
                T, std::initializer_list<U> &, Args &&...>> * = nullptr>
  void emplace(std::initializer_list<U> il, Args &&...args) {
    if (has_value()) {
      T t(il, std::forward<Args>(args)...);
      val() = std::move(t);
    } else {
      err().~unexpected<E>();
      new (valptr()) T(il, std::forward<Args>(args)...);
      this->m_has_val = true;
    }
  }

  /// @brief Emplace initializer list construct an expected value.
  ///
  /// @tparam U Type of initializer list elements.
  /// @tparam Args Types of `E`'s constructor arguments.
  /// @param il Forwarded constructor initializer list for `E`.
  /// @param args Forwarded constructor arguments for `E`.
  template <class U, class... Args,
            std::enable_if_t<!std::is_nothrow_constructible_v<
                T, std::initializer_list<U> &, Args &&...>> * = nullptr>
  void emplace(std::initializer_list<U> il, Args &&...args) {
    if (has_value()) {
      T t(il, std::forward<Args>(args)...);
      val() = std::move(t);
    } else {
      err().~unexpected<E>();
      new (valptr()) T(il, std::forward<Args>(args)...);
      this->m_has_val = true;
    }
  }

  /// @brief Swap this expected with another.
  ///
  /// @param rhs The expected to swap with this.
  void swap(expected &rhs) noexcept(
      std::is_nothrow_move_constructible_v<T> &&
      noexcept(swap(std::declval<T &>(), std::declval<T &>())) &&
      std::is_nothrow_move_constructible_v<E> &&
      noexcept(swap(std::declval<E &>(), std::declval<E &>()))) {
    if (has_value() && rhs.has_value()) {
      using std::swap;
      swap(val(), rhs.val());
    } else if (!has_value() && rhs.has_value()) {
      using std::swap;
      swap(err(), rhs.err());
    } else if (has_value()) {
      auto temp = std::move(rhs.err());
      new (rhs.valptr()) T(val());
      new (errptr()) unexpected_type(std::move(temp));
      std::swap(this->m_has_val, rhs.m_has_val);
    } else {
      auto temp = std::move(this->err());
      new (valptr()) T(rhs.val());
      new (errptr()) unexpected_type(std::move(temp));
      std::swap(this->m_has_val, rhs.m_has_val);
    }
  }

  /// @brief Access expected value members.
  ///
  /// @note Requires a value is stored.
  ///
  /// @return Returns a pointer to the stored value.
  constexpr const_pointer operator->() const { return valptr(); }

  constexpr pointer operator->() { return valptr(); }

  /// @brief Access the expected value.
  ///
  /// @note Requires a value is stored.
  ///
  /// @return Returns a const reference to the stored value.
  template <class U = T, std::enable_if_t<!std::is_void_v<U>> * = nullptr>
  constexpr const U &operator*() const & {
    return val();
  }

  /// @brief Access the expected value.
  ///
  /// @note Requires a value is stored.
  ///
  /// @return Returns a reference to the stored value.
  template <class U = T, std::enable_if_t<!std::is_void_v<U>> * = nullptr>
  constexpr U &operator*() & {
    return val();
  }

  /// @brief Access the expected value.
  ///
  /// @note Requires a value is stored.
  ///
  /// @return Returns a const r-value reference to the stored value.
  template <class U = T, std::enable_if_t<!std::is_void_v<U>> * = nullptr>
  constexpr const U &&operator*() const && {
    return std::move(val());
  }

  /// @brief Access the expected value.
  ///
  /// @note Requires a value is stored.
  ///
  /// @return Returns an r-value reference to the stored value.
  template <class U = T, std::enable_if_t<!std::is_void_v<U>> * = nullptr>
  constexpr U &&operator*() && {
    return std::move(val());
  }

  /// @brief Determine whether or not the optional has a value.
  ///
  /// @return Returns true is the expected has a value, false otherwise.
  constexpr bool has_value() const noexcept { return this->m_has_val; }

  constexpr explicit operator bool() const noexcept { return this->m_has_val; }

  /// @brief Access the expected value.
  ///
  /// @return Returns a const reference to the contained value if there is one,
  /// assert otherwise.
  template <class U = T, std::enable_if_t<!std::is_void_v<U>> * = nullptr>
  constexpr const U &value() const & {
    CARGO_ASSERT(has_value(), "bad expected access");
    return val();
  }

  /// @brief Access the expected value.
  ///
  /// @return Returns a reference to the contained value if there is one,
  /// assert otherwise.
  template <class U = T, std::enable_if_t<!std::is_void_v<U>> * = nullptr>
  constexpr U &value() & {
    CARGO_ASSERT(has_value(), "bad expected access");
    return val();
  }

  /// @brief Access the expected value.
  ///
  /// @return Returns a const r-value reference to the contained value if there
  /// is one, assert otherwise.
  template <class U = T, std::enable_if_t<!std::is_void_v<U>> * = nullptr>
  constexpr const U &&value() const && {
    CARGO_ASSERT(has_value(), "bad expected access");
    return std::move(val());
  }

  /// @brief Access the expected value.
  ///
  /// @return Returns an r-value reference to the contained value if there is
  /// one, assert otherwise.
  template <class U = T, std::enable_if_t<!std::is_void_v<U>> * = nullptr>
  constexpr U &&value() && {
    CARGO_ASSERT(has_value(), "bad expected access");
    return std::move(val());
  }

  /// @brief Access the unexpected value.
  ///
  /// @note Requires there is an unexpected value.
  ///
  /// @return Returns a const reference to the unexpected value.
  constexpr const E &error() const & { return err().value(); }

  /// @brief Access the unexpected value.
  ///
  /// @note Requires there is an unexpected value.
  ///
  /// @return Returns a reference to the unexpected value.
  constexpr E &error() & { return err().value(); }

  /// @brief Access the unexpected value.
  ///
  /// @note Requires there is an unexpected value.
  ///
  /// @return Returns a const r-value reference to the unexpected value.
  constexpr const E &&error() const && { return std::move(err().value()); }

  /// @brief Access the unexpected value.
  ///
  /// @note Requires there is an unexpected value.
  ///
  /// @return Returns an r-value reference to the unexpected value.
  constexpr E &&error() && { return std::move(err().value()); }

  /// @brief Access the expected value or the provided default value.
  ///
  /// @param v The default value to fallback to.
  ///
  /// @returns Returns a const reference to the stored value if there is one,
  /// otherwise returns the default value.
  template <class U>
  constexpr T value_or(U &&v) const & {
    static_assert(
        std::is_copy_constructible_v<T> && std::is_convertible_v<U &&, T>,
        "T must be copy-constructible and convertible to from U&&");
    return bool(*this) ? **this : static_cast<T>(std::forward<U>(v));
  }

  /// @brief Access the expected value or the provided default value.
  ///
  /// @param v The default value to fallback to.
  ///
  /// @returns Returns an r-value reference to the stored value if there is
  /// one, otherwise returns the default value.
  template <class U>
  constexpr T value_or(U &&v) && {
    static_assert(
        std::is_move_constructible_v<T> && std::is_convertible_v<U &&, T>,
        "T must be move-constructible and convertible to from U&&");
    return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(v));
  }

 private:
  /// @brief Get a pointer to expected value storage.
  ///
  /// @return Returns a pointer to the value storage.
  std::remove_reference_t<T> *valptr() {
    // If T is a reference we need to unwrap it from std::reference_wrapper by
    // assigning if to a reference.
    std::remove_reference_t<T> &ref = this->m_val;
    return std::addressof(ref);
  }

  /// @brief Get a pointer to unexpected value storage.
  ///
  /// @return Returns a pointer to the unexpected value storage.
  unexpected<E> *errptr() { return std::addressof(this->m_unexpect); }

  /// @brief Get the expected value.
  ///
  /// @tparam U Type of the expected value.
  ///
  /// @return Returns a reference to the expected value.
  template <class U = T, std::enable_if_t<!std::is_void_v<U>> * = nullptr>
  U &val() {
    return this->m_val;
  }

  /// @brief Get the unexpected value.
  ///
  /// @return Returns a reference to the unexpected value.
  unexpected<E> &err() { return this->m_unexpect; }

  /// @brief Get the expected value.
  ///
  /// @tparam U Type of the expected value.
  ///
  /// @return Returns a const reference to the expected value.
  template <class U = T, std::enable_if_t<!std::is_void_v<U>> * = nullptr>
  const U &val() const {
    return this->m_val;
  }

  /// @brief Get the unexpected value.
  ///
  /// @return Returns a const reference to the unexpected value.
  const unexpected<E> &err() const { return this->m_unexpect; }
};

/// @}

namespace detail {
template <class Exp>
using exp_t = typename std::decay_t<Exp>::value_type;

template <class Exp>
using err_t = typename std::decay_t<Exp>::error_type;

template <class Exp, class Ret>
using ret_t = expected<Ret, err_t<Exp>>;

template <class Exp, class F,
          class Ret = decltype(cargo::invoke(std::declval<F>(),
                                             *std::declval<Exp>())),
          std::enable_if_t<!std::is_void_v<exp_t<Exp>>> * = nullptr>
constexpr auto and_then_impl(Exp &&exp, F &&f) {
  static_assert(detail::is_expected_v<Ret>, "F must return an expected");
  return exp.has_value()
             ? cargo::invoke(std::forward<F>(f), *std::forward<Exp>(exp))
             : Ret(unexpect, exp.error());
}

template <class Exp, class F,
          class Ret = decltype(cargo::invoke(std::declval<F>(),
                                             *std::declval<Exp>())),
          std::enable_if_t<std::is_void_v<exp_t<Exp>>> * = nullptr>
constexpr auto and_then_impl(Exp &&exp, F &&f) {
  static_assert(detail::is_expected_v<Ret>, "F must return an expected");
  return exp.has_value() ? cargo::invoke(std::forward<F>(f))
                         : Ret(unexpect, exp.error());
}

template <class Exp, class F,
          class Ret = decltype(cargo::invoke(std::declval<F>(),
                                             *std::declval<Exp>())),
          std::enable_if_t<!std::is_void_v<Ret>> * = nullptr>
constexpr auto expected_map_impl(Exp &&exp, F &&f) {
  using result = ret_t<Exp, std::decay_t<Ret>>;
  return exp.has_value() ? result(cargo::invoke(std::forward<F>(f),
                                                *std::forward<Exp>(exp)))
                         : result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          class Ret = decltype(cargo::invoke(std::declval<F>(),
                                             *std::declval<Exp>())),
          std::enable_if_t<std::is_void_v<Ret>> * = nullptr>
auto expected_map_impl(Exp &&exp, F &&f) {
  using result = expected<void, err_t<Exp>>;
  if (exp.has_value()) {
    cargo::invoke(std::forward<F>(f), *std::forward<Exp>(exp));
    return result();
  }

  return result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          class Ret = decltype(cargo::invoke(std::declval<F>(),
                                             std::declval<Exp>().error())),
          std::enable_if_t<!std::is_void_v<Ret>> * = nullptr>
constexpr auto map_error_impl(Exp &&exp, F &&f) {
  using result = expected<exp_t<Exp>, std::decay_t<Ret>>;
  return exp.has_value()
             ? result(*std::forward<Exp>(exp))
             : result(unexpect, cargo::invoke(std::forward<F>(f),
                                              std::forward<Exp>(exp).error()));
}
template <class Exp, class F,
          class Ret = decltype(cargo::invoke(std::declval<F>(),
                                             std::declval<Exp>().error())),
          std::enable_if_t<std::is_void_v<Ret>> * = nullptr>
auto map_error_impl(Exp &&exp, F &&f) {
  using result = expected<exp_t<Exp>, monostate>;
  if (exp.has_value()) {
    return result(*std::forward<Exp>(exp));
  }
  cargo::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
  return result(unexpect, monostate{});
}

// NOLINTBEGIN(modernize-type-traits)
template <class Exp, class F,
          class Ret = decltype(cargo::invoke(std::declval<F>(),
                                             std::declval<Exp>().error())),
          std::enable_if_t<!std::is_void_v<Ret>> * = nullptr>
// NOLINTEND(modernize-type-traits)
constexpr auto or_else_impl(Exp &&exp, F &&f) {
  static_assert(detail::is_expected_v<Ret>, "F must return an expected");
  return exp.has_value() ? std::forward<Exp>(exp)
                         : cargo::invoke(std::forward<F>(f),
                                         std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          class Ret = decltype(cargo::invoke(std::declval<F>(),
                                             std::declval<Exp>().error())),
          std::enable_if_t<std::is_void_v<Ret>> * = nullptr>
std::decay_t<Exp> or_else_impl(Exp &&exp, F &&f) {
  return exp.has_value() ? std::forward<Exp>(exp)
                         : (cargo::invoke(std::forward<F>(f),
                                          std::forward<Exp>(exp).error()),
                            std::forward<Exp>(exp));
}

}  // namespace detail

/// @addtogroup cargo
/// @{

/// @brief Determine if an expected is equal to another.
///
/// @tparam T Type of `lhs` expected value.
/// @tparam E Type of `lhs` unexpected value.
/// @tparam U Type of `rhs` expected value.
/// @tparam F Type of `rhs` unexpected value.
/// @param lhs Left hand side expected to compare.
/// @param rhs Right hand side expected to compare.
///
/// @return Returns true if the objects are equal, false otherwise.
template <class T, class E, class U, class F>
constexpr bool operator==(const expected<T, E> &lhs,
                          const expected<U, F> &rhs) {
  if (lhs.has_value()) {
    return rhs.has_value() && *lhs == *rhs;
  } else {
    return !rhs.has_value() && lhs.error() == rhs.error();
  }
}

/// @brief Determine if an expected is not equal to another.
///
/// @tparam T Type of `lhs` expected value.
/// @tparam E Type of `lhs` unexpected value.
/// @tparam U Type of `rhs` expected value.
/// @tparam F Type of `rhs` unexpected value.
/// @param lhs Left hand side expected to compare.
/// @param rhs Right hand side expected to compare.
///
/// @return Returns true if the objects are not equal, false otherwise.
template <class T, class E, class U, class F>
constexpr bool operator!=(const expected<T, E> &lhs,
                          const expected<U, F> &rhs) {
  if (lhs.has_value()) {
    return !rhs.has_value() || *lhs != *rhs;
  } else {
    return rhs.has_value() || lhs.error() != rhs.error();
  }
}

/// @brief Determine if an expected is equal to a value.
///
/// @tparam T Type of `lhs` expected value.
/// @tparam E Type of `lhs` unexpected value.
/// @tparam U Type of value to compare.
/// @param x The expected to compare.
/// @param v The value to compare.
///
/// @return Returns true if the values are equal, false otherwise.
template <class T, class E, class U>
constexpr bool operator==(const expected<T, E> &x, const U &v) {
  return x.has_value() ? *x == v : false;
}

/// @brief Determine if an expected is equal to a value.
///
/// @tparam T Type of `lhs` expected value.
/// @tparam E Type of `lhs` unexpected value.
/// @tparam U Type of value to compare.
/// @param v The value to compare.
/// @param x The expected to compare.
///
/// @return Returns true if the values are equal, false otherwise.
template <class T, class E, class U>
constexpr bool operator==(const U &v, const expected<T, E> &x) {
  return x.has_value() ? *x == v : false;
}

/// @brief Determine if an expected is not equal to a value.
///
/// @tparam T Type of `lhs` expected value.
/// @tparam E Type of `lhs` unexpected value.
/// @tparam U Type of value to compare.
/// @param x The expected to compare.
/// @param v The value to compare.
///
/// @return Returns true if the values are not equal, false otherwise.
template <class T, class E, class U>
constexpr bool operator!=(const expected<T, E> &x, const U &v) {
  return x.has_value() ? *x != v : true;
}

/// @brief Determine if an expected is not equal to a value.
///
/// @tparam T Type of `lhs` expected value.
/// @tparam E Type of `lhs` unexpected value.
/// @tparam U Type of value to compare.
/// @param v The value to compare.
/// @param x The expected to compare.
///
/// @return Returns true if the values are not equal, false otherwise.
template <class T, class E, class U>
constexpr bool operator!=(const U &v, const expected<T, E> &x) {
  return x.has_value() ? *x != v : true;
}

/// @brief Determine if an expected is equal to an unexpected value.
///
/// @tparam T Type of `lhs` expected value.
/// @tparam E Type of `lhs` unexpected value.
/// @param x The expected to compare.
/// @param e The value to compare.
///
/// @return Returns true if the unexpected values are equal, false otherwise.
template <class T, class E>
constexpr bool operator==(const expected<T, E> &x, const unexpected<E> &e) {
  return x.has_value() ? false : x.error() == e.value();
}

/// @brief Determine if an expected is equal to an unexpected value.
///
/// @tparam T Type of `lhs` expected value.
/// @tparam E Type of `lhs` unexpected value.
/// @param e The value to compare.
/// @param x The expected to compare.
///
/// @return Returns true if the unexpected values are equal, false otherwise.
template <class T, class E>
constexpr bool operator==(const unexpected<E> &e, const expected<T, E> &x) {
  return x.has_value() ? false : x.error() == e.value();
}

/// @brief Determine if an expected is not equal to an unexpected value.
///
/// @tparam T Type of `lhs` expected value.
/// @tparam E Type of `lhs` unexpected value.
/// @param x The expected to compare.
/// @param e The value to compare.
///
/// @return Returns true if the unexpected values are not equal, false
/// otherwise.
template <class T, class E>
constexpr bool operator!=(const expected<T, E> &x, const unexpected<E> &e) {
  return x.has_value() ? true : x.error() != e.value();
}

/// @brief Determine if an expected is not equal to an unexpected value.
///
/// @tparam T Type of `lhs` expected value.
/// @tparam E Type of `lhs` unexpected value.
/// @param e The value to compare.
/// @param x The expected to compare.
///
/// @return Returns true if the unexpected values are not equal, false
/// otherwise.
template <class T, class E>
constexpr bool operator!=(const unexpected<E> &e, const expected<T, E> &x) {
  return x.has_value() ? true : x.error() != e.value();
}

/// @brief Swap an expected with another.
///
/// @tparam T Type of the expected value.
/// @tparam E Type of the unexpected value.
///
/// @param lhs Left hand expected to swap.
/// @param rhs Right hand expected to swap.
template <class T, class E,
          std::enable_if_t<std::is_move_constructible_v<T> &&
                           std::is_move_constructible_v<E>> * = nullptr>
void swap(expected<T, E> &lhs,
          expected<T, E> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

/// @}
}  // namespace cargo

#endif  // CARGO_EXPECTED_H_INCLUDED
