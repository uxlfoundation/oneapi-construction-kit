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
/// @brief Function invocation which supports anything callable.

#ifndef CARGO_FUNCTIONAL_H_INCLUDED
#define CARGO_FUNCTIONAL_H_INCLUDED

#include <cargo/type_traits.h>

#include <functional>

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief Invoke the Callable object `f` with the parameters `args`. Supports
/// pointers to members and member functions as well as functions and types with
/// overloaded call operator.
///
/// @param f A Callable object (e.g. function pointer, function object, member
/// function pointer)
/// @param args Arguments to call the function with
///
/// @return The result of calling `f` with the given arguments.
template <
    typename Fn, typename... Args,
    std::enable_if_t<std::is_member_pointer_v<std::decay_t<Fn>>> * = nullptr>
constexpr auto invoke(Fn &&f, Args &&...args) noexcept(
    noexcept(std::mem_fn(f)(std::forward<Args>(args)...)))
    -> decltype(std::mem_fn(f)(std::forward<Args>(args)...)) {
  return std::mem_fn(f)(std::forward<Args>(args)...);
}

/// @brief Invoke the Callable object `f` with the parameters `args`. Supports
/// pointers to members and member functions as well as functions and types with
/// overloaded call operator.
///
/// @param f A Callable object (e.g. function pointer, function object, member
/// function pointer)
/// @param args Arguments to call the function with
///
/// @return The result of calling `f` with the given arguments.
template <
    typename Fn, typename... Args,
    std::enable_if_t<!std::is_member_pointer_v<std::decay_t<Fn>>> * = nullptr>
constexpr auto invoke(Fn &&f, Args &&...args) noexcept(
    noexcept(std::forward<Fn>(f)(std::forward<Args>(args)...)))
    -> decltype(std::forward<Fn>(f)(std::forward<Args>(args)...)) {
  return std::forward<Fn>(f)(std::forward<Args>(args)...);
}

/// @}

namespace detail {
template <class F, class, class... Us>
struct invoke_result_impl;

template <class F, class... Us>
struct invoke_result_impl<
    F,
    decltype(cargo::invoke(std::declval<F>(), std::declval<Us>()...), void()),
    Us...> {
  using type =
      decltype(cargo::invoke(std::declval<F>(), std::declval<Us>()...));
};
}  // namespace detail

/// @addtogroup cargo
/// @{

/// @brief Get the return type of calling `cargo::invoke` with the given
/// arguments.
template <class F, class... Us>
using invoke_result = detail::invoke_result_impl<F, void, Us...>;

/// @brief Alias for `cargo::invoke_result` to reduce noise.
template <class F, class... Us>
using invoke_result_t = typename invoke_result<F, Us...>::type;

/// @}

namespace detail {
template <class, class R, class F, class... Args>
struct is_invocable_r_impl : std::false_type {};

template <class R, class F, class... Args>
struct is_invocable_r_impl<
    typename std::is_convertible<invoke_result_t<F, Args...>, R>::type, R, F,
    Args...> : std::true_type {};
}  // namespace detail

/// @addtogroup cargo
/// @{

/// @brief Determines whether F can be invoked using `cargo::invoke` with the
/// given arguments and yields a result that is convertible to R.
template <class R, class F, class... Args>
using is_invocable_r =
    detail::is_invocable_r_impl<std::true_type, R, F, Args...>;

/// @brief Conditionally wrap a reference.
///
/// @tparam T Type to wrap if it is a reference.
template <class T>
using wrap_reference_t =
    std::conditional_t<std::is_reference_v<T>,
                       std::reference_wrapper<std::remove_reference_t<T>>, T>;

/// @}
}  // namespace cargo

#endif
