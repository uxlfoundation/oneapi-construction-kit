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

/// @file
///
/// @brief Shared machinery for testing cargo.

#ifndef CARGO_COMMON_H_INCLUDED
#define CARGO_COMMON_H_INCLUDED

#include <utility>

struct copyable_t {
  copyable_t(int value) : value(value) {}
  copyable_t(const copyable_t &other) { value = other.value; }
  copyable_t(copyable_t &&) = delete;
  ~copyable_t() {}
  copyable_t &operator=(const copyable_t &other) {
    value = other.value;
    return *this;
  }
  int get() const { return value; }

private:
  int value;
};

struct movable_t {
  movable_t(int value) : value(value) {}
  movable_t(const movable_t &) = delete;
  movable_t(movable_t &&other) { value = other.value; }
  ~movable_t() {}
  movable_t &operator=(movable_t &&other) {
    value = std::move(other.value);
    return *this;
  }
  int get() const { return value; }

private:
  int value;
};

#endif // CARGO_COMMON_H_INCLUDED
