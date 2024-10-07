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
// Derived from CC0-licensed `tl::optional` library which can be found
// at https://github.com/TartanLlama/optional.  See optional.LICENSE.txt.

/// @file
///
/// @brief Implementation details of `cargo::optional`.

#ifndef CARGO_DETAIL_OPTIONAL_H_INCLUDED
#define CARGO_DETAIL_OPTIONAL_H_INCLUDED

#include <cargo/detail/sfinae_bases.h>
#include <cargo/functional.h>
#include <cargo/type_traits.h>
#include <cargo/utility.h>

namespace cargo {
template <class T>
class optional;

namespace detail {
// Trait for checking if a type is a cargo::optional
template <class T>
struct is_optional_impl : std::false_type {};
template <class T>
struct is_optional_impl<optional<T>> : std::true_type {};
template <class T>
using is_optional = is_optional_impl<std::decay_t<T>>;

// Change void to cargo::monostate
template <class U>
using fixup_void = std::conditional_t<std::is_void_v<U>, monostate, U>;

template <class F, class U, class = invoke_result_t<F, U>>
using get_map_return = optional<fixup_void<invoke_result_t<F, U>>>;

// Check if invoking F for some Us returns void
template <class F, class = void, class... U>
struct returns_void_impl;
template <class F, class... U>
struct returns_void_impl<F, std::void_t<invoke_result_t<F, U...>>, U...>
    : std::is_void<invoke_result_t<F, U...>> {};
template <class F, class... U>
using returns_void = returns_void_impl<F, void, U...>;

template <class T, class... U>
using enable_if_ret_void = std::enable_if_t<returns_void<T &&, U...>::value>;

template <class T, class... U>
using disable_if_ret_void = std::enable_if_t<!returns_void<T &&, U...>::value>;

template <class T, class U>
using enable_forward_value =
    std::enable_if_t<std::is_constructible_v<T, U &&> &&
                     !std::is_same_v<std::decay_t<U>, in_place_t> &&
                     !std::is_same_v<optional<T>, std::decay_t<U>>>;

template <class T, class U, class Other>
using enable_from_other =
    std::enable_if_t<std::is_constructible_v<T, Other> &&
                     !std::is_constructible_v<T, optional<U> &> &&
                     !std::is_constructible_v<T, optional<U> &&> &&
                     !std::is_constructible_v<T, const optional<U> &> &&
                     !std::is_constructible_v<T, const optional<U> &&> &&
                     !std::is_convertible_v<optional<U> &, T> &&
                     !std::is_convertible_v<optional<U> &&, T> &&
                     !std::is_convertible_v<const optional<U> &, T> &&
                     !std::is_convertible_v<const optional<U> &&, T>>;

template <class T, class U>
using enable_assign_forward = std::enable_if_t<
    !std::is_same_v<optional<T>, std::decay_t<U>> &&
    !(std::is_scalar_v<T> && std::is_same_v<T, std::decay_t<U>>) &&
    std::is_constructible_v<T, U> && std::is_assignable_v<T &, U>>;

template <class T, class U, class Other>
using enable_assign_from_other =
    std::enable_if_t<std::is_constructible_v<T, Other> &&
                     std::is_assignable_v<T &, Other> &&
                     !std::is_constructible_v<T, optional<U> &> &&
                     !std::is_constructible_v<T, optional<U> &&> &&
                     !std::is_constructible_v<T, const optional<U> &> &&
                     !std::is_constructible_v<T, const optional<U> &&> &&
                     !std::is_convertible_v<optional<U> &, T> &&
                     !std::is_convertible_v<optional<U> &&, T> &&
                     !std::is_convertible_v<const optional<U> &, T> &&
                     !std::is_convertible_v<const optional<U> &&, T> &&
                     !std::is_assignable_v<T &, optional<U> &> &&
                     !std::is_assignable_v<T &, optional<U> &&> &&
                     !std::is_assignable_v<T &, const optional<U> &> &&
                     !std::is_assignable_v<T &, const optional<U> &&>>;

// The storage base manages the actual storage, and correctly propagates
// trivial destruction from T This case is for when T is trivially
// destructible
template <class T, bool = std::is_trivially_destructible_v<T>>
struct optional_storage_base {
  constexpr optional_storage_base() : m_dummy(0), m_has_value(false) {}

  template <class... U>
  constexpr optional_storage_base(in_place_t, U &&...u)
      : m_value(std::forward<U>(u)...), m_has_value(true) {}

  ~optional_storage_base() {
    if (m_has_value) {
      m_value.~T();
      m_has_value = false;
    }
  }

  union {
    char m_dummy;
    T m_value;
  };

  bool m_has_value;
};

// This case is for when T is not trivially destructible
template <class T>
struct optional_storage_base<T, true> {
  constexpr optional_storage_base() : m_dummy(0), m_has_value(false) {}

  template <class... U>
  constexpr optional_storage_base(in_place_t, U &&...u)
      : m_value(std::forward<U>(u)...), m_has_value(true) {}

  // No destructor, so this class is trivially destructible

  union {
    char m_dummy;
    T m_value;
  };

  bool m_has_value = false;
};

// This base class provides some handy member functions which can be used in
// further derived classes
template <class T>
struct optional_operations_base : optional_storage_base<T> {
  using optional_storage_base<T>::optional_storage_base;

  void hard_reset() {
    get().~T();
    this->m_has_value = false;
  }

  template <class... Args>
  void construct(Args &&...args) {
    new (static_cast<void *>(std::addressof(this->m_value)))
        T(std::forward<Args>(args)...);
    this->m_has_value = true;
  }

  template <class Opt>
  void assign(Opt &&rhs) {
    if (this->has_value()) {
      if (rhs.has_value()) {
        this->m_value = std::forward<Opt>(rhs).get();
      } else {
        this->m_value.~T();
        this->m_has_value = false;
      }
    }

    if (rhs.has_value()) {
      construct(std::forward<Opt>(rhs).get());
    }
  }

  bool has_value() const { return this->m_has_value; }

  constexpr T &get() & { return this->m_value; }
  constexpr const T &get() const & { return this->m_value; }
  constexpr T &&get() && { return std::move(this->m_value); }
  constexpr const T &&get() const && { return std::move(this->m_value); }
};

// This class manages conditionally having a trivial copy constructor
// This specialization is for when T is trivially copy constructible
template <class T, bool = std::is_trivially_copy_constructible_v<T>>
struct optional_copy_base : optional_operations_base<T> {
  using optional_operations_base<T>::optional_operations_base;
};

// This specialization is for when T is not trivially copy constructible
template <class T>
struct optional_copy_base<T, false> : optional_operations_base<T> {
  using optional_operations_base<T>::optional_operations_base;

  optional_copy_base() = default;
  optional_copy_base(const optional_copy_base &rhs)
      : optional_operations_base<T>() {
    if (rhs.has_value()) {
      this->construct(rhs.get());
    } else {
      this->m_has_value = false;
    }
  }

  optional_copy_base(optional_copy_base &&rhs) = default;
  optional_copy_base &operator=(const optional_copy_base &rhs) = default;
  optional_copy_base &operator=(optional_copy_base &&rhs) = default;
};

// This class manages conditionally having a trivial move constructor
template <class T, bool = std::is_trivially_move_constructible_v<T>>
struct optional_move_base : optional_copy_base<T> {
  using optional_copy_base<T>::optional_copy_base;
};

template <class T>
struct optional_move_base<T, false> : optional_copy_base<T> {
  using optional_copy_base<T>::optional_copy_base;

  optional_move_base() = default;
  optional_move_base(const optional_move_base &rhs) = default;

  optional_move_base(optional_move_base &&rhs) {
    if (rhs.has_value()) {
      this->construct(std::move(rhs.get()));
    } else {
      this->m_has_value = false;
    }
  }
  optional_move_base &operator=(const optional_move_base &rhs) = default;
  optional_move_base &operator=(optional_move_base &&rhs) = default;
};

// This class manages conditionally having a trivial copy assignment operator
template <class T, bool = std::is_trivially_copy_assignable_v<T> &&
                          std::is_trivially_copy_constructible_v<T> &&
                          std::is_trivially_destructible_v<T>>
struct optional_copy_assign_base : optional_move_base<T> {
  using optional_move_base<T>::optional_move_base;
};

template <class T>
struct optional_copy_assign_base<T, false> : optional_move_base<T> {
  using optional_move_base<T>::optional_move_base;

  optional_copy_assign_base() = default;
  optional_copy_assign_base(const optional_copy_assign_base &rhs) = default;

  optional_copy_assign_base(optional_copy_assign_base &&rhs) = default;
  optional_copy_assign_base &operator=(const optional_copy_assign_base &rhs) {
    this->assign(rhs);
    return *this;
  }
  optional_copy_assign_base &operator=(optional_copy_assign_base &&rhs) =
      default;
};

// This class manages conditionally having a trivial move assignment operator
template <class T, bool = std::is_trivially_destructible_v<T> &&
                          std::is_trivially_move_constructible_v<T> &&
                          std::is_trivially_move_assignable_v<T>>
struct optional_move_assign_base : optional_copy_assign_base<T> {
  using optional_copy_assign_base<T>::optional_copy_assign_base;
};

template <class T>
struct optional_move_assign_base<T, false> : optional_copy_assign_base<T> {
  using optional_copy_assign_base<T>::optional_copy_assign_base;

  optional_move_assign_base() = default;
  optional_move_assign_base(const optional_move_assign_base &rhs) = default;

  optional_move_assign_base(optional_move_assign_base &&rhs) = default;
  optional_move_assign_base &operator=(const optional_move_assign_base &rhs) =
      default;
  optional_move_assign_base &operator=(optional_move_assign_base &&rhs) {
    this->assign(std::move(rhs));
    return *this;
  }
};
}  // namespace detail
}  // namespace cargo

#endif  // CARGO_DETAIL_OPTIONAL_H_INCLUDED
