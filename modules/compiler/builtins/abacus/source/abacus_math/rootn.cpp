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
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/floor_unsafe.h>
#include <abacus/internal/horner_polynomial.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  static T initial(const T x) {
    // Approximation of log2(x+1)/x over range [-0.5; 0]
    // See rootn.sollya for derivation
    const abacus_half polynomial[7] = {4.1875f16,   -9.5f16,    18.0625f16,
                                       -22.6719f16, 17.5938f16, -7.65625f16,
                                       1.42871f16};

    const T x_minus_one = x - 1.0f;

    return x_minus_one * abacus::internal::horner_polynomial(x, polynomial);
  }

  static T refinement(const T x) {
    // Approximation of (2^x-1)/x over range [0; 1]
    // See rootn.sollya for derivation, with 1.0 term inserted at beginning
    const abacus_half polynomial[4] = {1.0f16, 0.693359375f16, 0.234375f16,
                                       7.177734375e-2f16};

    return abacus::internal::horner_polynomial(x, polynomial);
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T initial(const T x) {
    const abacus_float polynomial[7] = {4.18622064838195f, -9.50029573356452f,
                                        18.0589370154565f, -22.6661106036793f,
                                        17.5891648877289f, -7.65432946100401f,
                                        1.42910968791821f};

    const T x_minus_one = x - 1.0f;

    return x_minus_one * abacus::internal::horner_polynomial(x, polynomial);
  }

  static T refinement(const T x) {
    const abacus_float polynomial[6] = {
        .999999925066056f,    .693153073167932f,    .240153617206963f,
        .558263175864784e-1f, .898934063766142e-2f, .187757646702639e-2f};

    return abacus::internal::horner_polynomial(x, polynomial);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
// see maple worksheet for polynomial derivation
static ABACUS_CONSTANT abacus_double polynomialD[19] = {
    0.14426950408889637655e1, -0.72134752044397218070e0,
    0.48089834708328874843e0, -0.36067374897596112851e0,
    0.28853956240100904639e0, -0.2404325557196992561e0,
    0.20642924772278778666e0, -0.1757559046306388131e0,
    0.2064158768153297520e0,  0.200638406099893077e0,
    0.2077396790753492396e1,  0.8232974580315425626e1,
    0.2741388724783965889e2,  0.67441067604121061693e2,
    0.12462289939522620496e3, 0.16607608888895043741e3,
    0.15225060938993225896e3, 0.85990788318382575009e2,
    0.23050555733213757242e2};

template <typename T>
struct helper<T, abacus_double> {
  static T initial(const T x) {
    const T x_minus_one = x - 1.0;

    return x_minus_one *
           abacus::internal::horner_polynomial(x_minus_one, polynomialD);
  }

  static T refinement(const T x) {
    const abacus_double polynomial[12] = {1.0,
                                          0.6931471805599453234921813e0,
                                          0.2402265069590972163658798e0,
                                          0.5550410866496438513834959e-1,
                                          0.9618129105363013736198286e-2,
                                          0.1333355833004708123997947e-2,
                                          0.1540352174282662596355535e-3,
                                          0.1525298689419357566275462e-4,
                                          0.1321075362706774161704002e-5,
                                          0.1023456783619446173448029e-6,
                                          0.6641338398972727973820141e-8,
                                          0.6109234053107283700972839e-9};

    return abacus::internal::horner_polynomial(x, polynomial);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T rootn(
    const T x,
    const typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type &n) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
      IntVecType;

  const T xAbs = __abacus_fabs(x);

  IntVecType xExp;
  // Get mantissa in range [1/2, 1)
  const T xMant = __abacus_frexp(xAbs, &xExp);

  const T log2_xMant = helper<T>::initial(xMant);

  const IntVecType nFudged = __abacus_select(n, 1, n == 0);

  const IntVecType initial_guess = xExp / nFudged;  // int divide
  xExp -= (initial_guess * n);  // xExp = xExp (mod n) basically

  T sum = (log2_xMant + abacus::detail::cast::convert<T>(xExp)) /
          abacus::detail::cast::convert<T>(nFudged);

  // get the floor value of sum:
  IntVecType exponent = abacus::detail::cast::convert<IntVecType>(
      abacus::internal::floor_unsafe(sum));
  T mantissa = sum - abacus::detail::cast::convert<T>(exponent);

  exponent += initial_guess;

  T result = helper<T>::refinement(mantissa);

  result = __abacus_ldexp(result, exponent);

  const SignedType n_odd =
      abacus::detail::cast::convert<SignedType>((n & 0x1) == 0x1);
  const SignedType ans_is_negative =
      abacus::detail::cast::convert<SignedType>(n_odd & (x < 0.0f));

  result = __abacus_select(result, -result, ans_is_negative);

  result = __abacus_select(result, (T)0.0f, (SignedType)(x == 0.0f));

  const T furtherCond1 = __abacus_select(
      (T)ABACUS_INFINITY, __abacus_copysign((T)ABACUS_INFINITY, x), n_odd);

  const SignedType cond1 =
      (x == 0.0f) & abacus::detail::cast::convert<SignedType>(n < 0);
  result = __abacus_select(result, furtherCond1, cond1);

  const T furtherCond2 =
      __abacus_select((T)ABACUS_INFINITY, (T)-ABACUS_INFINITY, ans_is_negative);
  const SignedType cond2 = __abacus_isinf(x);
  result = __abacus_select(result, furtherCond2, cond2);

  const SignedType cond3 =
      __abacus_isinf(x) & abacus::detail::cast::convert<SignedType>(n < 0);
  result = __abacus_select(result, (T)0.0f, cond3);

  const SignedType nan_cond = __abacus_isnan(x) | (~n_odd & (x < 0)) |
                              abacus::detail::cast::convert<SignedType>(n == 0);

  result = __abacus_select(result, FPShape<T>::NaN(), nan_cond);
  return result;
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_rootn(abacus_half x, abacus_int n) {
  return rootn<>(x, n);
}
abacus_half2 ABACUS_API __abacus_rootn(abacus_half2 x, abacus_int2 n) {
  return rootn<>(x, n);
}
abacus_half3 ABACUS_API __abacus_rootn(abacus_half3 x, abacus_int3 n) {
  return rootn<>(x, n);
}
abacus_half4 ABACUS_API __abacus_rootn(abacus_half4 x, abacus_int4 n) {
  return rootn<>(x, n);
}
abacus_half8 ABACUS_API __abacus_rootn(abacus_half8 x, abacus_int8 n) {
  return rootn<>(x, n);
}
abacus_half16 ABACUS_API __abacus_rootn(abacus_half16 x, abacus_int16 n) {
  return rootn<>(x, n);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_rootn(abacus_float x, abacus_int n) {
  return rootn<>(x, n);
}
abacus_float2 ABACUS_API __abacus_rootn(abacus_float2 x, abacus_int2 n) {
  return rootn<>(x, n);
}
abacus_float3 ABACUS_API __abacus_rootn(abacus_float3 x, abacus_int3 n) {
  return rootn<>(x, n);
}
abacus_float4 ABACUS_API __abacus_rootn(abacus_float4 x, abacus_int4 n) {
  return rootn<>(x, n);
}
abacus_float8 ABACUS_API __abacus_rootn(abacus_float8 x, abacus_int8 n) {
  return rootn<>(x, n);
}
abacus_float16 ABACUS_API __abacus_rootn(abacus_float16 x, abacus_int16 n) {
  return rootn<>(x, n);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_rootn(abacus_double x, abacus_int n) {
  return rootn<>(x, n);
}
abacus_double2 ABACUS_API __abacus_rootn(abacus_double2 x, abacus_int2 n) {
  return rootn<>(x, n);
}
abacus_double3 ABACUS_API __abacus_rootn(abacus_double3 x, abacus_int3 n) {
  return rootn<>(x, n);
}
abacus_double4 ABACUS_API __abacus_rootn(abacus_double4 x, abacus_int4 n) {
  return rootn<>(x, n);
}
abacus_double8 ABACUS_API __abacus_rootn(abacus_double8 x, abacus_int8 n) {
  return rootn<>(x, n);
}
abacus_double16 ABACUS_API __abacus_rootn(abacus_double16 x, abacus_int16 n) {
  return rootn<>(x, n);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
