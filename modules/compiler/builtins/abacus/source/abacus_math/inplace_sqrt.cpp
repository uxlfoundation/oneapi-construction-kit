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

#include <abacus/abacus_config.h>
#include <abacus/abacus_type_traits.h>

#include <cmath>

namespace abacus {
namespace detail {
template <typename T>
void inplace_sqrt(T &t) {
  using ET = typename TypeTraits<T>::ElementType;
  for (ET *p = reinterpret_cast<ET *>(&t), *e = reinterpret_cast<ET *>(&t + 1);
       p != e; ++p) {
    *p = std::sqrt(*p);
  }
}
}  // namespace detail
}  // namespace abacus

#ifdef __CA_BUILTINS_HALF_SUPPORT
template void abacus::detail::inplace_sqrt(abacus_half &);
template void abacus::detail::inplace_sqrt(abacus_half2 &);
template void abacus::detail::inplace_sqrt(abacus_half3 &);
template void abacus::detail::inplace_sqrt(abacus_half4 &);
template void abacus::detail::inplace_sqrt(abacus_half8 &);
template void abacus::detail::inplace_sqrt(abacus_half16 &);
#endif  // __CA_BUILTINS_HALF_SUPPORT

template void abacus::detail::inplace_sqrt(abacus_float &);
template void abacus::detail::inplace_sqrt(abacus_float2 &);
template void abacus::detail::inplace_sqrt(abacus_float3 &);
template void abacus::detail::inplace_sqrt(abacus_float4 &);
template void abacus::detail::inplace_sqrt(abacus_float8 &);
template void abacus::detail::inplace_sqrt(abacus_float16 &);

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template void abacus::detail::inplace_sqrt(abacus_double &);
template void abacus::detail::inplace_sqrt(abacus_double2 &);
template void abacus::detail::inplace_sqrt(abacus_double3 &);
template void abacus::detail::inplace_sqrt(abacus_double4 &);
template void abacus::detail::inplace_sqrt(abacus_double8 &);
template void abacus::detail::inplace_sqrt(abacus_double16 &);
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
