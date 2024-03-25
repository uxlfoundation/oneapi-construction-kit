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

#ifndef UR_INFO_H_INCLUDED
#define UR_INFO_H_INCLUDED

#include <cassert>
#include <cstring>
#include <type_traits>

#include "ur_api.h"

namespace ur {
/// @brief Helper function for setting the memory at an address to a specific
/// value, useful for reducing boiler plate in device/platform queries.
///
/// @tparam T The type of the value being set, must be trivial and not char *.
/// @param[in] pSize Size of the type `T`
/// @param[in] pInfo The address containing the memory to be set.
/// @param[in] value The value to set the memory to.
///
/// @return Result indicating the success of the operation.
template <class T>
inline ur_result_t setInfo(size_t propSize, void *pInfo, const T value,
                           size_t *pPropSizeRet) {
  static_assert(std::is_trivial_v<T>, "T must be a trivial type");
  static_assert(!std::is_same_v<T, char *>,
                "T must not be 'char *', use 'const char *' instead");
  if (pInfo && propSize != sizeof(T)) {
    return UR_RESULT_ERROR_INVALID_SIZE;
  }
  if (pPropSizeRet) {
    *pPropSizeRet = sizeof(T);
  }
  if (pInfo) {
    std::memcpy(pInfo, &value, propSize);
  }
  return UR_RESULT_SUCCESS;
}

/// @brief Template specialization of setInfo for c-style const char * strings.
template <>
inline ur_result_t setInfo<const char *>(size_t propSize, void *pInfo,
                                         const char *str,
                                         size_t *pPropSizeRet) {
  const size_t size = std::strlen(str) + 1;
  if (pInfo && size != propSize) {
    return UR_RESULT_ERROR_INVALID_SIZE;
  }
  if (pPropSizeRet) {
    *pPropSizeRet = size;
  }
  if (pInfo) {
    std::strncpy(static_cast<char *>(pInfo), str, propSize);
  }
  return UR_RESULT_SUCCESS;
}
}  // namespace ur

#endif  // UR_INFO_H_INCLUDED
