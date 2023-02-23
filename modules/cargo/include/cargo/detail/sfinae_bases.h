// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CARGO_SHARED_H_INCLUDED
#define CARGO_SHARED_H_INCLUDED

namespace cargo {
namespace detail {

// delete_ctor_base will conditionally delete copy and move constructors
// depending on the value of EnableCopy and EnableMove.
template <bool EnableCopy, bool EnableMove>
struct delete_ctor_base {
  delete_ctor_base() = default;
  delete_ctor_base(const delete_ctor_base &) = default;
  delete_ctor_base(delete_ctor_base &&) noexcept = default;
  delete_ctor_base &operator=(const delete_ctor_base &) = default;
  delete_ctor_base &operator=(delete_ctor_base &&) noexcept = default;
};

template <>
struct delete_ctor_base<true, false> {
  delete_ctor_base() = default;
  delete_ctor_base(const delete_ctor_base &) = default;
  delete_ctor_base(delete_ctor_base &&) noexcept = delete;
  delete_ctor_base &operator=(const delete_ctor_base &) = default;
  delete_ctor_base &operator=(delete_ctor_base &&) noexcept = default;
};

template <>
struct delete_ctor_base<false, true> {
  delete_ctor_base() = default;
  delete_ctor_base(const delete_ctor_base &) = delete;
  delete_ctor_base(delete_ctor_base &&) noexcept = default;
  delete_ctor_base &operator=(const delete_ctor_base &) = default;
  delete_ctor_base &operator=(delete_ctor_base &&) noexcept = default;
};

template <>
struct delete_ctor_base<false, false> {
  delete_ctor_base() = default;
  delete_ctor_base(const delete_ctor_base &) = delete;
  delete_ctor_base(delete_ctor_base &&) noexcept = delete;
  delete_ctor_base &operator=(const delete_ctor_base &) = default;
  delete_ctor_base &operator=(delete_ctor_base &&) noexcept = default;
};

// delete_assign_base will conditionally delete copy and move constructors
// depending on the value of EnableCopy and EnableMove.
template <bool EnableCopy, bool EnableMove>
struct delete_assign_base {
  delete_assign_base() = default;
  delete_assign_base(const delete_assign_base &) = default;
  delete_assign_base(delete_assign_base &&) noexcept = default;
  delete_assign_base &operator=(const delete_assign_base &) = default;
  delete_assign_base &operator=(delete_assign_base &&) noexcept = default;
};

template <>
struct delete_assign_base<true, false> {
  delete_assign_base() = default;
  delete_assign_base(const delete_assign_base &) = default;
  delete_assign_base(delete_assign_base &&) noexcept = default;
  delete_assign_base &operator=(const delete_assign_base &) = default;
  delete_assign_base &operator=(delete_assign_base &&) noexcept = delete;
};

template <>
struct delete_assign_base<false, true> {
  delete_assign_base() = default;
  delete_assign_base(const delete_assign_base &) = default;
  delete_assign_base(delete_assign_base &&) noexcept = default;
  delete_assign_base &operator=(const delete_assign_base &) = delete;
  delete_assign_base &operator=(delete_assign_base &&) noexcept = default;
};

template <>
struct delete_assign_base<false, false> {
  delete_assign_base() = default;
  delete_assign_base(const delete_assign_base &) = default;
  delete_assign_base(delete_assign_base &&) noexcept = default;
  delete_assign_base &operator=(const delete_assign_base &) = delete;
  delete_assign_base &operator=(delete_assign_base &&) noexcept = delete;
};

}  // namespace detail
}  // namespace cargo

#endif  // CARGO_SHARED_H_INCLUDED
