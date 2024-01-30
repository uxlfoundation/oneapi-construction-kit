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
// Derived from CC0-licensed `tl::function_ref` library which can be found
// at https://github.com/TartanLlama/function_ref.  See
// function_ref.LICENSE.txt.

/// @file
///
/// @brief An implementation of the proposed std::function_ref with extensions.

#ifndef CARGO_FUNCTION_REF_INCLUDED
#define CARGO_FUNCTION_REF_INCLUDED

#include <cargo/functional.h>

#include <functional>
#include <utility>

namespace cargo {

/// A lightweight non-owning reference to a callable.
///
/// Example usage:
///
/// ```cpp
/// void foo (function_ref<int(int)> func) {
///     std::cout << "Result is " << func(21); //42
/// }
///
/// foo([](int i) { return i*2; });
/// ```
template <class F>
class function_ref;

/// Specialization for function types.
template <class R, class... Args>
class function_ref<R(Args...)> {
 public:
  constexpr function_ref() noexcept = delete;

  /// Creates a `function_ref` which refers to the same callable as `rhs`.
  constexpr function_ref(const function_ref<R(Args...)> &rhs) noexcept =
      default;

  /// Constructs a `function_ref` referring to `f`.
  ///
  /// \synopsis template <typename F> constexpr function_ref(F &&f) noexcept
  template <
      typename F,
      std::enable_if_t<!std::is_same_v<std::decay_t<F>, function_ref> &&
                       is_invocable_r<R, F &&, Args...>::value> * = nullptr>
  constexpr function_ref(F &&f) noexcept
      : obj_(const_cast<void *>(
            reinterpret_cast<const void *>(std::addressof(f)))) {
    callback_ = [](void *obj, Args... args) -> R {
      return cargo::invoke(*reinterpret_cast<std::add_pointer_t<F>>(obj),
                           std::forward<Args>(args)...);
    };
  }

  /// Makes `*this` refer to the same callable as `rhs`.
  constexpr function_ref<R(Args...)> &operator=(
      const function_ref<R(Args...)> &rhs) noexcept = default;

  /// Makes `*this` refer to `f`.
  ///
  /// \synopsis template <typename F> constexpr function_ref &operator=(F &&f)
  /// noexcept;
  template <
      typename F,
      std::enable_if_t<is_invocable_r<R, F &&, Args...>::value> * = nullptr>
  constexpr function_ref<R(Args...)> &operator=(F &&f) noexcept {
    obj_ = reinterpret_cast<void *>(std::addressof(f));
    callback_ = [](void *obj, Args... args) {
      return cargo::invoke(*reinterpret_cast<std::add_pointer_t<F>>(obj),
                           std::forward<Args>(args)...);
    };

    return *this;
  }

  /// Swaps the referred callables of `*this` and `rhs`.
  constexpr void swap(function_ref<R(Args...)> &rhs) noexcept {
    std::swap(obj_, rhs.obj_);
    std::swap(callback_, rhs.callback_);
  }

  /// Call the stored callable with the given arguments.
  R operator()(Args... args) const {
    return callback_(obj_, std::forward<Args>(args)...);
  }

 private:
  void *obj_ = nullptr;
  R (*callback_)(void *, Args...) = nullptr;
};

/// Swaps the referred callables of `lhs` and `rhs`.
template <typename R, typename... Args>
constexpr void swap(function_ref<R(Args...)> &lhs,
                    function_ref<R(Args...)> &rhs) noexcept {
  lhs.swap(rhs);
}

#if __cplusplus >= 201703L
template <typename R, typename... Args>
function_ref(R (*)(Args...)) -> function_ref<R(Args...)>;

// TODO, will require some kind of callable traits
// template <typename F>
// function_ref(F) -> function_ref</* deduced if possible */>;
#endif
}  // namespace cargo

#endif
