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

#ifndef __ABACUS_INTERNAL_ATAN_UNSAFE_H__
#define __ABACUS_INTERNAL_ATAN_UNSAFE_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/horner_polynomial.h>

namespace abacus {
namespace internal {
template <typename T>
T atan_unsafe(const T &x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  // if negative, use identity atan(-x) = -atan(x)

  const T a = __abacus_fabs(x);

  const SignedType large = a > 1.0;

  // if large, use identity atan(1 / x) = pi/2 - atan(x)
  const T b = __abacus_select(a, (T)1.0 / a, large);

  // PI/7 in two constants for Cody & Waite reduction
  const abacus_double k1 =
      0.448798950512827605494663340468500412028167057053586545853563;
  const abacus_double k2 =
      1.749495427353350041202816705705358654585356351318683091E-17;

  const SignedType range = b > k1;

  // need to clamp down into range (0..pi/7] for our polynomial
  // we use the identity atan(z + k) = atan(z) + atan(k)
  // where z = (x - k) / (1 + x * k)
  const T c =
      __abacus_select(b, ((b - k1) - k2) / ((b * k1 + 1) + b * k2), range);

  // see maple worksheet for how polynomial was derived
  const abacus_double polynomial[15] = {
      0.99999999999999999999998635870534739e0,
      -0.33333333333333333329328450816353372e0,
      0.19999999999999998048033917861754675e0,
      -0.14285714285713909979821631347979628e0,
      0.11111111111073204503794369643921582e0,
      -0.90909090886036177161475009007507255e-1,
      0.76923076006587753056588231472047744e-1,
      -0.66666641635509259718256348574956167e-1,
      0.58823044993214798103727818944914713e-1,
      -0.52624819577808016998563813504250130e-1,
      0.47550605172893112210848362092563464e-1,
      -0.42977410991113660198961051685398211e-1,
      0.37392689026993304453000106391402972e-1,
      -0.27671732461794832889713721816369215e-1,
      0.12384767226415412063082059444905478e-1};

  T r = c * abacus::internal::horner_polynomial(c * c, polynomial);

  // atan(PI/7) in two constants for Cody & Waite reduction
  const abacus_double atank1 =
      0.421854683596300300030547010851915742171171496091474076014410;
  const abacus_double atank2 =
      5.029588852653915742171171496091474076014410538343604331E-18;

  r = __abacus_select(r, (r + atank1) + atank2, range);

  // pi/2 in three constants for extended Cody & Waite reduction
  const abacus_double piHi = 1.57079632679489655799898173427E0;
  const abacus_double piMi = 6.12323399573697434445160507072E-17;
  const abacus_double piLo = 7.9975825339924874821929915098E-33;

  r = __abacus_select(r, -(((r - piHi) - piMi) - piLo), large);

  return __abacus_copysign(r, x);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_ATAN_UNSAFE_H__
