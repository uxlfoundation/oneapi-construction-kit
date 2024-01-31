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
/// @brief A std::thread wrapper with benefits.

#ifndef CARGO_THREAD_H_INCLUDED
#define CARGO_THREAD_H_INCLUDED

#include <string>
#include <thread>

#include "cargo/error.h"
#include "cargo/thread_safety.h"

namespace cargo {
/// @brief A std::thread wrapper with benefits.
///
/// @see https://en.cppreference.com/w/cpp/thread/thread
///
/// In addition to capabilities of std::thread, this wrapper also enables the
/// getting and setting of the thread name on supported POSIX systems or on
/// Windows 10+.
struct thread {
  using id = std::thread::id;
  using native_handle_type = std::thread::native_handle_type;

  /// @brief Default constructor.
  thread() noexcept = default;

  /// @brief Move constructor.
  ///
  /// @param other Thread to be moved from.
  thread(thread &&other) noexcept = default;

  thread(const thread &) = delete;

  /// @brief Creates a new object associated with a thread of execution.
  ///
  /// @see https://en.cppreference.com/w/cpp/thread/thread/thread
  ///
  /// @tparam Function Type of the callable to invoke.
  /// @tparam Args Variadic type parameter pack of `Function` arguments.
  /// @param function The callable to invoke in the thread of execution.
  /// @param args The variable parameter pack to pass to `function`.
  template <class Function, class... Args>
  explicit thread(Function &&function, Args &&...args)
      : Thread{std::forward<Function>(function), std::forward<Args>(args)...} {}

  /// @brief Destructor.
  ~thread() = default;

  /// @brief Move assignement operator.
  ///
  /// @see https://en.cppreference.com/w/cpp/thread/thread/operator%3D
  ///
  /// @param other Thread to be moved from.
  ///
  /// @return Returns a reference to this cargo::thread.
  thread &operator=(thread &&other) noexcept {
    Thread = std::move(other.Thread);
    return *this;
  }

  /// @brief Checks wheather the thread is joinable.
  ///
  /// @see https://en.cppreference.com/w/cpp/thread/thread/joinable
  ///
  /// @return Returns `true` is the thread can be joined, `false` otherwise.
  bool joinable() const noexcept { return Thread.joinable(); }

  /// @brief Get the ID of the thread.
  ///
  /// @see https://en.cppreference.com/w/cpp/thread/thread/get_id
  ///
  /// @return Returns the ID of the thead.
  cargo::thread::id get_id() const noexcept { return Thread.get_id(); }

  /// @brief Get the underlying implementation-defined thread handle.
  ///
  /// @see https://en.cppreference.com/w/cpp/thread/thread/native_handle
  ///
  /// @return Returns the implementation-defined thread handle.
  native_handle_type native_handle() { return Thread.native_handle(); }

  /// @brief Get the number of concurrent hardware threads available.
  ///
  /// @see https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency
  ///
  /// @return Returns the number of concurrent threads supported by the
  /// implementation.
  static unsigned int hardware_concurrency() noexcept {
    return std::thread::hardware_concurrency();
  }

  /// @brief Waits for the thread to finish its execution.
  ///
  /// @see https://en.cppreference.com/w/cpp/thread/thread/join
  void join() { Thread.join(); }

  /// @brief Permits the thread to execute independently from the thread
  /// handle.
  ///
  /// @see https://en.cppreference.com/w/cpp/thread/thread/detach
  void detach() { Thread.detach(); }

  /// @brief Swaps two thread objects.
  ///
  /// @see https://en.cppreference.com/w/cpp/thread/thread/swap
  ///
  /// @param other Thread to be swapped with.
  void swap(cargo::thread &other) noexcept {
    std::thread temp = std::move(Thread);
    Thread = std::move(other.Thread);
    other.Thread = std::move(temp);
  }

  /// @brief Set the thread name.
  ///
  /// @param name Name to set the thread to.
  ///
  /// @return Returns the result of the attempts to set the thread name.
  /// @retval `cargo::success` if the thread name was set successfully.
  /// @retval `cargo::out_of_bounds` if `name` is too long.
  /// @retval `cargo::unknown_error` if an OS specific error occurs.
  /// @retval `cargo::unsupported` if not supported.
  cargo::result set_name(const std::string &name) noexcept;

  /// @brief Get the thread name.
  ///
  /// @return Returns the thread name or an error.
  /// @retval `cargo::unknown_error` if an OS specific error occurs.
  /// @retval `cargo::unsupported` if not supported.
  [[nodiscard]] cargo::error_or<std::string> get_name() noexcept;

  /// @brief Operator for implict conversion to std::thread.
  operator std::thread &() { return Thread; }

 private:
  std::thread Thread;
};
}  // namespace cargo

namespace std {
template <>
inline void swap<cargo::thread>(cargo::thread &lhs,
                                cargo::thread &rhs) noexcept {
  lhs.swap(rhs);
}
}  // namespace std

#endif  // CARGO_THREAD_H_INCLUDED
