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
/// @brief

#ifndef UR_BASE_H_INCLUDED
#define UR_BASE_H_INCLUDED

#include <atomic>
#include <type_traits>

#include "ur_api.h"

namespace ur {
/// @brief Reference counting base class from which unified runtime API objects
/// can inherit so they don't need to reimplement reference counting machinery.
///
/// TODO: This reference counting base class isn't ideal. Some kind of block
/// allocator per handle type, which can be used to validate ownership/lifetime
/// of objects, and avoid numerous calls to system allocators would be nicer.
/// Another issue with this approach is any handles which are not released
/// aren't automatically cleaned up at exit, while that makes it easy (with
/// ASAN) to find memory leaks in applications, maybe isn't best behaviour.
struct base {
  /// @brief Constructor
  ///
  /// Initializes starting reference count to 1 at creation.
  base() : count{1} {}

  /// @brief The reference count of this object.
  std::atomic_uint32_t count;
};

/// @brief Increment the reference count of an object.
///
/// @tparam Handle the type of the object, must be child of base.
///
/// @param[in] object Object to increase reference count on.
///
/// @return Error code indicating success of retain operation.
template <class Handle>
inline ur_result_t retain(Handle *object) {
  static_assert(std::is_base_of_v<ur::base, Handle>,
                "Handle must inerit from ur::base");
  object->count++;
  return UR_RESULT_SUCCESS;
}

/// @brief Decremented the reference count of an object.
///
/// @tparam Handle the type of the object, must be child of base.
///
/// @param[in] object Object to decrease reference count on.
///
/// @return Error code indicating success of retain operation.
template <class Handle>
ur_result_t release(Handle *object) {
  static_assert(std::is_base_of_v<ur::base, Handle>,
                "Handle must inerit from ur::base");
  auto count = --object->count;
  if (count == 0) {
    delete object;
  }
  return UR_RESULT_SUCCESS;
}
}  // namespace ur

#endif  // UR_BASE_H_INCLUDED
