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
/// @brief

#ifndef MUX_UTILS_SMALL_VECTOR_H_INCLUDES
#define MUX_UTILS_SMALL_VECTOR_H_INCLUDES

#include <cargo/small_vector.h>
#include <mux/utils/allocator.h>

namespace mux {
/// @brief Specialization of `cargo::small_vector` using a Mux allocator.
///
/// ```cpp
/// void example(mux_allocator_info_t allocator_info) {
///   mux::small_vector<int, 8> ints(allocator_info);
///   // use as normal...
/// }
/// ```
///
/// @see `cargo::small_vector<T, N, A>` documentation.
///
/// @tparam T Type of contained elements.
/// @tparam N Capacity of the embedded storage.
template <class T, size_t N>
using small_vector = cargo::small_vector<T, N, cargo_allocator<T>>;
}  // namespace mux

#endif  // MUX_UTILS_SMALL_VECTOR_H_INCLUDES
