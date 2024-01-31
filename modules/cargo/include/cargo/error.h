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
/// @brief Error handling utilities.

#ifndef CARGO_ERROR_H_INCLUDED
#define CARGO_ERROR_H_INCLUDED

#include <cargo/attributes.h>
#include <cargo/functional.h>
#include <cargo/type_traits.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

#if __GNUC__
#define CARGO_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif _MSC_VER
#define CARGO_PRETTY_FUNCTION __FUNCSIG__
#else
#define CARGO_PRETTY_FUNCTION ""
#endif

/// @addtogroup cargo
/// @{

#ifndef NDEBUG
#define CARGO_ASSERT(CONDITION, MESSAGE)                                      \
  if (!(CONDITION)) {                                                         \
    (void)std::fprintf(stderr, "%s:%d: assert: %s\n  %s: %s\n", __FILE__,     \
                       __LINE__, #CONDITION, CARGO_PRETTY_FUNCTION, MESSAGE); \
    std::abort();                                                             \
  }                                                                           \
  (void)0
#else
#define CARGO_ASSERT(CONDITION, MESSAGE) (void)0
#endif

/// @}

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief Enumeration of cargo result codes.
enum result : int32_t {
  success,
  bad_alloc,
  bad_argument,
  out_of_bounds,
  overflow,
  unknown_error,
  unsupported,
};

/// @brief Error or value container.
///
/// The `cargo::error_or<T>` type should be used as the return type for
/// functions which may fail. The returned type can then be checked to see if
/// it contains an error or a valid value before continuing.
///
/// Example code:
///
/// ```cpp
/// #include <cargo/dynamic_array.h>
/// #include <cargo/error.h>
///
/// cargo::error_or<cargo::dynamic_array<int>> getDynArray() {
///   cargo::dynamic_array<int> dynArray;
///   if (auto error = dynArray.alloc(10)) {
///     return error;
///   }
///   return dynArray;
/// }
/// ```
///
/// @tparam T Type of the contained value.
template <class T>
class error_or {
 public:
  using value_type = std::remove_reference_t<T>;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;

 private:
  /// @brief Either `value_type` or `std::reference_wrapper<value_type>`.
  using value_storage_type = wrap_reference_t<T>;

 public:
  /// @brief Error constructor.
  ///
  /// @param error Type of error.
  error_or(cargo::result error) : HasError(true) {
    CARGO_ASSERT(cargo::success != error, "error must not be success");
    new (&ErrorStorage) cargo::result(error);
  }

  /// @brief Default value constructor.
  error_or() : HasError(false) { new (&ValueStorage) value_storage_type(); }

  /// @brief Argument value constructor.
  ///
  /// @tparam First Type of the first argument, the constructor will be disabled
  /// if `sizeof...(Args)` is zero and this type is `cargo::error_or &`. It is
  /// required to do this so that `cargo::error_or &`'s will be copy constructed
  /// as expected instead of being consumed by this constructor.
  /// @tparam Args Variadic argument types.
  /// @param first First argument.
  /// @param args Variadic arguments.
  template <class First, class... Args,
            class = std::enable_if_t<
                !(sizeof...(Args) == 0 &&
                  std::is_same_v<error_or, std::remove_reference_t<First>>)>>
  error_or(First &&first, Args &&...args) : HasError(false) {
    new (&ValueStorage) value_storage_type(std::forward<First>(first),
                                           std::forward<Args>(args)...);
  }

  /// @brief Copy constructor.
  ///
  /// @param other Other instance to copy.
  error_or(const error_or &other) : HasError(other.HasError) {
    if (HasError) {
      new (&ErrorStorage) cargo::result(other.ErrorStorage);
    } else {
      new (&ValueStorage) value_storage_type(other.ValueStorage);
    }
  }

  /// @brief Move constructor.
  ///
  /// @param other Other instance to move.
  error_or(error_or &&other) : HasError(other.HasError) {
    if (HasError) {
      new (&ErrorStorage) cargo::result(other.ErrorStorage);
    } else {
      new (&ValueStorage) value_storage_type(std::move(other.ValueStorage));
    }
  }

  /// @brief Destructor.
  ~error_or() {
    if (!HasError) {
      ValueStorage.~value_storage_type();
    }
  }

  /// @brief Copy assignment operator.
  ///
  /// @param other Other instance to copy.
  ///
  /// @return Returns a reference to this object.
  error_or &operator=(const error_or &other) {
    new (this) error_or(other);
    return *this;
  }

  /// @brief Move assignment operator.
  ///
  /// @param other Other instance to move.
  ///
  /// @return Returns a reference to this object.
  error_or &operator=(error_or &&other) {
    new (this) error_or(std::move(other));
    return *this;
  }

  /// @brief Determine if a value or error is held.
  ///
  /// @return Returns true is a value is held, false otherwise.
  operator bool() const { return !HasError; }

  /// @brief Access the error if no value is held.
  ///
  /// @return Returns the error if there is one, `cargo::success` otherwise.
  cargo::result error() const {
    return HasError ? ErrorStorage : cargo::success;
  }

  /// @brief Access a reference to the value.
  ///
  /// If an error is held this results in undefined behaviour.
  ///
  /// @return Returns a reference to the value.
  reference operator*() {
    CARGO_ASSERT(!HasError, "contains error");
    // KLOCWORK "LOCRET.RET" possible false positive
    // UB if the returned reference is accessed after the error_or object goes
    // out of scope.
    return ValueStorage;
  }

  /// @brief Access a const reference to the value.
  ///
  /// If an error is held this results in undefined behaviour.
  ///
  /// @return Returns a const reference to the value.
  const_reference operator*() const {
    CARGO_ASSERT(!HasError, "contains error");
    // KLOCWORK "LOCRET.RET" possible false positive
    // UB if the returned reference is accessed after the error_or object goes
    // out of scope.
    return ValueStorage;
  }

  /// @brief Access a pointer to the value.
  ///
  /// If an error is held this results in undefined behaviour.
  ///
  /// @return Returns a pointer to the value.
  pointer operator->() {
    CARGO_ASSERT(!HasError, "contains error");
    reference ref = ValueStorage;
    return std::addressof(ref);
  }

  /// @brief Access a const pointer to the value.
  ///
  /// If an error is held this results in undefined behaviour.
  ///
  /// @return Returns a const pointer to the value.
  const_pointer operator->() const {
    CARGO_ASSERT(!HasError, "contains error");
    const_reference ref = ValueStorage;
    return std::addressof(ref);
  }

 private:
  /// @brief Union of `cargo::result` and `value_storage_type`.
  union {
    cargo::result ErrorStorage;
    value_storage_type ValueStorage;
  };
  /// @brief True if an error is held, false otherwise.
  bool HasError : 1;
};

/// @}
}  // namespace cargo

#endif  // CARGO_ERROR_H_INCLUDED
