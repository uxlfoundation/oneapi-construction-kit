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
/// @brief Implementation details of `cargo::expected`.

#ifndef CARGO_DETAIL_EXPECTED_H_INCLUDED
#define CARGO_DETAIL_EXPECTED_H_INCLUDED

#include <cargo/detail/sfinae_bases.h>
#include <cargo/type_traits.h>
#include <cargo/utility.h>

namespace cargo {
template <class T, class E>
class expected;

template <class E>
class unexpected;

/// @brief A tag type to tell expected to construct the unexpected value
struct unexpect_t {
  unexpect_t() = default;
};

namespace detail {

// Trait for checking if a type is a cargo::expected
template <class T>
struct is_expected_impl : std::false_type {};
template <class T, class E>
struct is_expected_impl<expected<T, E>> : std::true_type {};
template <class T>
using is_expected = is_expected_impl<std::decay_t<T>>;

template <class T, class E, class U>
using expected_enable_forward_value =
    std::enable_if_t<std::is_constructible_v<T, U &&> &&
                     !std::is_same_v<std::decay_t<U>, in_place_t> &&
                     !std::is_same_v<expected<T, E>, std::decay_t<U>> &&
                     !std::is_same_v<unexpected<E>, std::decay_t<U>>>;

template <class T, class E, class U, class G, class UR, class GR>
using expected_enable_from_other =
    std::enable_if_t<std::is_constructible_v<T, UR> &&
                     std::is_constructible_v<E, GR> &&
                     !std::is_constructible_v<T, expected<U, G> &> &&
                     !std::is_constructible_v<T, expected<U, G> &&> &&
                     !std::is_constructible_v<T, const expected<U, G> &> &&
                     !std::is_constructible_v<T, const expected<U, G> &&> &&
                     !std::is_convertible_v<expected<U, G> &, T> &&
                     !std::is_convertible_v<expected<U, G> &&, T> &&
                     !std::is_convertible_v<const expected<U, G> &, T> &&
                     !std::is_convertible_v<const expected<U, G> &&, T>>;

template <class T, class U>
using is_void_or = std::conditional_t<std::is_void_v<T>, std::true_type, U>;

template <class T>
using is_copy_constructible_or_void =
    is_void_or<T, std::is_copy_constructible<T>>;

template <class T>
using is_move_constructible_or_void =
    is_void_or<T, std::is_move_constructible<T>>;

struct no_init_t {};
static constexpr no_init_t no_init{};

// Implements the storage of the values, and ensures that the destructor is
// trivial if it can be.
//
// This specialization is for where neither `T` or `E` is trivially
// destructible, so the destructors must be called on destruction of the
// `expected`
template <class T, class E, bool = std::is_trivially_destructible_v<T>,
          bool = std::is_trivially_destructible_v<E>>
struct expected_storage_base {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_no_init(), m_has_val(false) {}

  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<T, Args &&...>> * = nullptr>
  constexpr expected_storage_base(in_place_t, Args &&...args)
      : m_val(std::forward<Args>(args)...), m_has_val(true) {}

  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                T, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il,
                                  Args &&...args)
      : m_val(il, std::forward<Args>(args)...), m_has_val(true) {}
  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<E, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                E, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  ~expected_storage_base() {
    if (m_has_val) {
      m_val.~T();
    } else {
      m_unexpect.~unexpected<E>();
    }
  }
  union {
    char m_no_init;
    T m_val;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

// This specialization is for when both `T` and `E` are trivially-destructible,
// so the destructor of the `expected` can be trivial.
template <class T, class E>
struct expected_storage_base<T, E, true, true> {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_no_init(), m_has_val(false) {}

  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<T, Args &&...>> * = nullptr>
  constexpr expected_storage_base(in_place_t, Args &&...args)
      : m_val(std::forward<Args>(args)...), m_has_val(true) {}

  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                T, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il,
                                  Args &&...args)
      : m_val(il, std::forward<Args>(args)...), m_has_val(true) {}
  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<E, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                E, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  ~expected_storage_base() = default;
  union {
    char m_no_init;
    T m_val;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

// T is trivial, E is not.
template <class T, class E>
struct expected_storage_base<T, E, true, false> {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_no_init(), m_has_val(false) {}

  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<T, Args &&...>> * = nullptr>
  constexpr expected_storage_base(in_place_t, Args &&...args)
      : m_val(std::forward<Args>(args)...), m_has_val(true) {}

  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                T, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il,
                                  Args &&...args)
      : m_val(il, std::forward<Args>(args)...), m_has_val(true) {}
  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<E, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                E, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  ~expected_storage_base() {
    if (!m_has_val) {
      m_unexpect.~unexpected<E>();
    }
  }

  union {
    char m_no_init;
    T m_val;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

// E is trivial, T is not.
template <class T, class E>
struct expected_storage_base<T, E, false, true> {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_no_init(), m_has_val(false) {}

  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<T, Args &&...>> * = nullptr>
  constexpr expected_storage_base(in_place_t, Args &&...args)
      : m_val(std::forward<Args>(args)...), m_has_val(true) {}

  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                T, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il,
                                  Args &&...args)
      : m_val(il, std::forward<Args>(args)...), m_has_val(true) {}
  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<E, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                E, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  ~expected_storage_base() {
    if (m_has_val) {
      m_val.~T();
    }
  }
  union {
    char m_no_init;
    T m_val;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

// `T` is `void`, `E` is trivially-destructible
template <class E>
struct expected_storage_base<void, E, false, true> {
  constexpr expected_storage_base() : m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_no_init(), m_has_val(false) {}

  constexpr expected_storage_base(in_place_t) : m_has_val(true) {}

  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<E, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                E, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  ~expected_storage_base() = default;
  union {
    char m_no_init;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

// `T` is `void`, `E` is not trivially-destructible
template <class E>
struct expected_storage_base<void, E, false, false> {
  constexpr expected_storage_base() : m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_no_init(), m_has_val(false) {}

  constexpr expected_storage_base(in_place_t) : m_has_val(true) {}

  template <
      class... Args,
      std::enable_if_t<std::is_constructible_v<E, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            std::enable_if_t<std::is_constructible_v<
                E, std::initializer_list<U> &, Args &&...>> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  ~expected_storage_base() {
    if (!m_has_val) {
      m_unexpect.~unexpected<E>();
    }
  }

  union {
    char m_no_init;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

// This base class provides some handy member functions which can be used in
// further derived classes
template <class T, class E>
struct expected_operations_base : expected_storage_base<T, E> {
  using expected_storage_base<T, E>::expected_storage_base;

  template <class... Args>
  void construct(Args &&...args) noexcept {
    new (std::addressof(this->m_val)) T(std::forward<Args>(args)...);
    this->m_has_val = true;
  }

  template <class Rhs>
  void construct_with(Rhs &&rhs) noexcept {
    new (std::addressof(this->m_val)) T(std::forward<Rhs>(rhs).get());
    this->m_has_val = true;
  }

  template <class... Args>
  void construct_error(Args &&...args) noexcept {
    new (std::addressof(this->m_unexpect))
        unexpected<E>(std::forward<Args>(args)...);
    this->m_has_val = false;
  }

  void assign(const expected_operations_base &rhs) noexcept {
    if (!this->m_has_val && rhs.m_has_val) {
      geterr().~unexpected<E>();
      construct(rhs.get());
    } else {
      assign_common(rhs);
    }
  }

  void assign(expected_operations_base &&rhs) noexcept {
    if (!this->m_has_val && rhs.m_has_val) {
      geterr().~unexpected<E>();
      construct(std::move(rhs).get());
    } else {
      assign_common(std::move(rhs));
    }
  }

  // The common part of move/copy assigning
  template <class Rhs>
  void assign_common(Rhs &&rhs) {
    if (this->m_has_val) {
      if (rhs.m_has_val) {
        get() = std::forward<Rhs>(rhs).get();
      } else {
        get().~T();
        construct_error(std::forward<Rhs>(rhs).geterr());
      }
    } else {
      if (!rhs.m_has_val) {
        geterr() = std::forward<Rhs>(rhs).geterr();
      }
    }
  }

  bool has_value() const { return this->m_has_val; }

  constexpr T &get() & { return this->m_val; }
  constexpr const T &get() const & { return this->m_val; }
  constexpr T &&get() && { return std::move(this->m_val); }
  constexpr const T &&get() const && { return std::move(this->m_val); }

  constexpr unexpected<E> &geterr() & { return this->m_unexpect; }
  constexpr const unexpected<E> &geterr() const & { return this->m_unexpect; }
  constexpr unexpected<E> &&geterr() && { return std::move(this->m_unexpect); }
  constexpr const unexpected<E> &&geterr() const && {
    return std::move(this->m_unexpect);
  }
};

// This base class provides some handy member functions which can be used in
// further derived classes
template <class E>
struct expected_operations_base<void, E> : expected_storage_base<void, E> {
  using expected_storage_base<void, E>::expected_storage_base;

  template <class... Args>
  void construct() noexcept {
    this->m_has_val = true;
  }

  // This function doesn't use its argument, but needs it so that code in
  // levels above this can work independencargoy of whether T is void
  template <class Rhs>
  void construct_with(Rhs &&) noexcept {
    this->m_has_val = true;
  }

  template <class... Args>
  void construct_error(Args &&...args) noexcept {
    new (std::addressof(this->m_unexpect))
        unexpected<E>(std::forward<Args>(args)...);
    this->m_has_val = false;
  }

  template <class Rhs>
  void assign(Rhs &&rhs) noexcept {
    if (!this->m_has_val) {
      if (rhs.m_has_val) {
        geterr().~unexpected<E>();
        construct();
      } else {
        geterr() = std::forward<Rhs>(rhs).geterr();
      }
    } else {
      if (!rhs.m_has_val) {
        construct_error(std::forward<Rhs>(rhs).geterr());
      }
    }
  }

  bool has_value() const { return this->m_has_val; }

  constexpr unexpected<E> &geterr() & { return this->m_unexpect; }
  constexpr const unexpected<E> &geterr() const & { return this->m_unexpect; }
  constexpr unexpected<E> &&geterr() && { std::move(this->m_unexpect); }
  constexpr const unexpected<E> &&geterr() const && {
    return std::move(this->m_unexpect);
  }
};

// This class manages conditionally having a trivial copy constructor
// This specialization is for when T and E are trivially copy constructible
template <class T, class E,
          bool =
              is_void_or<T, std::is_trivially_copy_constructible<T>>::value &&
              std::is_trivially_copy_constructible_v<E>>
struct expected_copy_base : expected_operations_base<T, E> {
  using expected_operations_base<T, E>::expected_operations_base;
};

// This specialization is for when T or E are not trivially copy constructible
template <class T, class E>
struct expected_copy_base<T, E, false> : expected_operations_base<T, E> {
  using expected_operations_base<T, E>::expected_operations_base;

  expected_copy_base() = default;
  expected_copy_base(const expected_copy_base &rhs)
      : expected_operations_base<T, E>(no_init) {
    if (rhs.has_value()) {
      this->construct_with(rhs);
    } else {
      this->construct_error(rhs.geterr());
    }
  }

  expected_copy_base(expected_copy_base &&rhs) = default;
  expected_copy_base &operator=(const expected_copy_base &rhs) = default;
  expected_copy_base &operator=(expected_copy_base &&rhs) = default;
};

// This class manages conditionally having a trivial move constructor
template <class T, class E,
          bool =
              is_void_or<T, std::is_trivially_move_constructible<T>>::value &&
              std::is_trivially_move_constructible_v<E>>
struct expected_move_base : expected_copy_base<T, E> {
  using expected_copy_base<T, E>::expected_copy_base;
};
template <class T, class E>
struct expected_move_base<T, E, false> : expected_copy_base<T, E> {
  using expected_copy_base<T, E>::expected_copy_base;

  expected_move_base() = default;
  expected_move_base(const expected_move_base &rhs) = default;

  expected_move_base(expected_move_base &&rhs) noexcept(
      std::is_nothrow_move_constructible_v<T>)
      : expected_copy_base<T, E>(no_init) {
    if (rhs.has_value()) {
      this->construct_with(std::move(rhs));
    } else {
      this->construct_error(std::move(rhs.geterr()));
    }
  }
  expected_move_base &operator=(const expected_move_base &rhs) = default;
  expected_move_base &operator=(expected_move_base &&rhs) = default;
};

// This class manages conditionally having a trivial copy assignment operator
template <
    class T, class E,
    bool =
        is_void_or<T, conjunction<std::is_trivially_copy_assignable<T>,
                                  std::is_trivially_copy_constructible<T>,
                                  std::is_trivially_destructible<T>>>::value &&
        std::is_trivially_copy_assignable_v<E> &&
        std::is_trivially_copy_constructible_v<E> &&
        std::is_trivially_destructible_v<E>>
struct expected_copy_assign_base : expected_move_base<T, E> {
  using expected_move_base<T, E>::expected_move_base;
};

template <class T, class E>
struct expected_copy_assign_base<T, E, false> : expected_move_base<T, E> {
  using expected_move_base<T, E>::expected_move_base;

  expected_copy_assign_base() = default;
  expected_copy_assign_base(const expected_copy_assign_base &rhs) = default;
  expected_copy_assign_base(expected_copy_assign_base &&rhs) = default;

  expected_copy_assign_base &operator=(const expected_copy_assign_base &rhs) {
    this->assign(rhs);
    return *this;
  }
  expected_copy_assign_base &operator=(expected_copy_assign_base &&rhs) =
      default;
};

// This class manages conditionally having a trivial move assignment operator
template <
    class T, class E,
    bool = is_void_or<
               T, conjunction<std::is_trivially_destructible<T>,
                              std::is_trivially_move_constructible<T>,
                              std::is_trivially_move_assignable<T>>>::value &&
           std::is_trivially_destructible_v<E> &&
           std::is_trivially_move_constructible_v<E> &&
           std::is_trivially_move_assignable_v<E>>
struct expected_move_assign_base : expected_copy_assign_base<T, E> {
  using expected_copy_assign_base<T, E>::expected_copy_assign_base;
};

template <class T, class E>
struct expected_move_assign_base<T, E, false>
    : expected_copy_assign_base<T, E> {
  using expected_copy_assign_base<T, E>::expected_copy_assign_base;

  expected_move_assign_base() = default;
  expected_move_assign_base(const expected_move_assign_base &rhs) = default;

  expected_move_assign_base(expected_move_assign_base &&rhs) = default;

  expected_move_assign_base &operator=(const expected_move_assign_base &rhs) =
      default;

  expected_move_assign_base &operator=(
      expected_move_assign_base
          &&rhs) noexcept(std::is_nothrow_move_constructible_v<T> &&
                          std::is_nothrow_move_assignable_v<T>) {
    this->assign(std::move(rhs));
    return *this;
  }
};

// This is needed to be able to construct the expected_default_ctor_base which
// follows, while still conditionally deleting the default constructor.
struct default_constructor_tag {
  explicit constexpr default_constructor_tag() = default;
};

// expected_default_ctor_base will ensure that expected has a deleted default
// constructor if T is not default constructible.
// This specialization is for when T is default constructible
template <class T, class E,
          bool Enable = std::is_default_constructible_v<T> || std::is_void_v<T>>
struct expected_default_ctor_base {
  constexpr expected_default_ctor_base() noexcept = default;
  constexpr expected_default_ctor_base(
      const expected_default_ctor_base &) noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base &&) noexcept =
      default;
  expected_default_ctor_base &operator=(
      const expected_default_ctor_base &) noexcept = default;
  expected_default_ctor_base &operator=(
      expected_default_ctor_base &&) noexcept = default;

  constexpr explicit expected_default_ctor_base(default_constructor_tag) {}
};

// This specialization is for when T is not default constructible
template <class T, class E>
struct expected_default_ctor_base<T, E, false> {
  constexpr expected_default_ctor_base() noexcept = delete;
  constexpr expected_default_ctor_base(
      const expected_default_ctor_base &) noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base &&) noexcept =
      default;
  expected_default_ctor_base &operator=(
      const expected_default_ctor_base &) noexcept = default;
  expected_default_ctor_base &operator=(
      expected_default_ctor_base &&) noexcept = default;

  constexpr explicit expected_default_ctor_base(default_constructor_tag) {}
};

}  // namespace detail
}  // namespace cargo

#endif  // CARGO_DETAIL_EXPECTED_H_INCLUDED
