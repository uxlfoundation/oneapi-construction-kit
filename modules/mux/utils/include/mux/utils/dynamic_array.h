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
/// @brief Mux allocator specialization of cargo::dynamic_array.

#ifndef MUX_UTILS_DYNAMIC_ARRAY_H_INCLUDED
#define MUX_UTILS_DYNAMIC_ARRAY_H_INCLUDED

#include "cargo/dynamic_array.h"
#include "mux/utils/allocator.h"

namespace mux {
/// @brief Specialization of `cargo::dynamic_array` using a Mux allocator.
///
/// ```cpp
/// void example(mux_allocator_info_t allocator_info) {
///   mux::dynamic_array<int> ints(allocator_info);
///   // use as normal...
/// }
/// ```
///
/// @tparam T Type of contained elements.
template <class T>
using dynamic_array = cargo::dynamic_array<T, cargo_allocator<T>>;
}  // namespace mux

#endif  // MUX_UTILS_DYNAMIC_ARRAY_H_INCLUDED
