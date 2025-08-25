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

#ifndef __ABACUS_INTERNAL_HALF_RANGE_REDUCTION_H__
#define __ABACUS_INTERNAL_HALF_RANGE_REDUCTION_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_type_traits.h>

namespace abacus {
namespace internal {
template <typename T>
inline T half_range_reduction(const T &x,
                              typename TypeTraits<T>::SignedType *out_octet) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  T xAbs = __abacus_fabs(x);

  SignedType octet = abacus::detail::cast::convert<SignedType>(
      xAbs * 1.27323949337005615234375f);  // 4/pi
  T octetF = abacus::detail::cast::convert<T>(octet);

  // cody waithe constants for pi/4:
  const T cw1 = 0.78515625f;
  const T cw2 = 2.4127960205078125e-4f;
  const T cw3 = 6.33299350738525390625e-7f;
  const T cw4 = 4.96046814735251473393873311579E-10f;

  T xReduced = xAbs;

  // a quick cody waithe to see if the octet is off. Doesn't catch all errors
  // but the remaining ones are small
  xReduced -= octetF * cw1;
  xReduced -= octetF * 2.419133961666e-4f;

  octet += __abacus_select((SignedType)0, (SignedType)1,
                           xReduced > 0.392699092626571f);  // pi/8
  octetF = abacus::detail::cast::convert<T>(octet);

  // more accurate cody waithe
  xReduced = xAbs;
  xReduced -= octetF * cw1;
  xReduced -= octetF * cw2;
  xReduced -= octetF * cw3;
  xReduced -= octetF * cw4;

  *out_octet = __abacus_select(octet, -octet, x < 0);

  return __abacus_select(xReduced, -xReduced, x < 0);
}

// half_range_reduction has a codegen bug on OpenCL for float3 types. Hack
// around it by casting the vec3 to a vec4 for the operation (see Redmine
// #8082).
#ifdef __OPENCL_VERSION__
inline abacus_float3 half_range_reduction(const abacus_float3 &x,
                                          abacus_int3 *out_octet) {
  const abacus_float4 f = __abacus_as_float4(x);
  abacus_int4 out_octet4;
  const abacus_float4 r = half_range_reduction(f, &out_octet4);

  *out_octet = out_octet4.xyz;
  return r.xyz;
}
#endif
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_HALF_RANGE_REDUCTION_H__
