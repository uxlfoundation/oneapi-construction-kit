// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
