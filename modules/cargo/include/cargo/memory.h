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

#ifndef CARGO_MEMORY_H_INCLUDED
#define CARGO_MEMORY_H_INCLUDED

#include <memory>

namespace cargo {

/// @brief Reimplementation of the C++17 std::uninitialized_move
template <class InputIterator, class OutputIterator>
InputIterator uninitialized_move(InputIterator first, InputIterator last,
                                 OutputIterator dst_first) {
  for (; first != last; ++first, ++dst_first) {
    new (static_cast<void *>(std::addressof(*dst_first)))
        typename std::iterator_traits<InputIterator>::value_type(
            std::move(*first));
  }
  return first;
}

} // namespace cargo

#endif // CARGO_MEMORY_H_INCLUDED
