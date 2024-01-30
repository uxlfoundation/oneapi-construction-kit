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

#ifndef __ABACUS_INTERNAL_FLOOR_UNSAFE_H__
#define __ABACUS_INTERNAL_FLOOR_UNSAFE_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace abacus {
namespace internal {
template <typename T>
inline typename TypeTraits<T>::SignedType floor_unsafe(const T &x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  const SignedType truncated = abacus::detail::cast::convert<SignedType>(x);
  T diff = __abacus_fabs(x - abacus::detail::cast::convert<T>(truncated));

  const SignedType condition =
      __abacus_isgreater(diff, 0) & __abacus_isless(x, 0);
  const SignedType decremented = truncated - 1;
  return __abacus_select(truncated, decremented, condition);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_FLOOR_UNSAFE_H__
