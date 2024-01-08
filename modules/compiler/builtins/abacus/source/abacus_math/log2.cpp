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
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/horner_polynomial.h>
namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  static T _(const T x) {
    // Calculated by sollya/log2.sollya.
    // Polynomial approximating the function log2(x+1)/x over the range
    // (1/sqrt(2))-1, sqrt(2)-1.
    const abacus_half polynomial[9] = {
        1.4423828125f16,   -0.72119140625f16,  0.5078125f16,
        -0.38623046875f16, -0.300537109375f16, 0.54736328125f16,
        4.06640625f16,     -6.3828125f16,      -1.4638671875f16,
    };

    return x * abacus::internal::horner_polynomial(x, polynomial);
  }
};
#endif

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    // see maple worksheet for polynomial derivation
    const abacus_float polynomial[11] = {+1.44269504f,
                                         -7.21347510814666748046875E-1f,
                                         +4.808981716632843017578125E-1f,
                                         -3.606741130352020263671875E-1f,
                                         +2.885643541812896728515625E-1f,
                                         -2.40459501743316650390625E-1f,
                                         +2.05218255519866943359375E-1f,
                                         -1.791259944438934326171875E-1f,
                                         +1.7119292914867401123046875E-1f,
                                         -1.675888001918792724609375E-1f,
                                         9.700144827365875244140625E-2f};

    return x * abacus::internal::horner_polynomial(x, polynomial);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
static ABACUS_CONSTANT abacus_double polynomialD[21] = {
    0.1442695040888963410896627e1,  -0.7213475204444823443799544e0,
    0.4808983469629818634732703e0,  -0.3606737602218485615533635e0,
    0.2885390081792205640230255e0,  -0.2404491735526678079859620e0,
    0.2060992914568305345730601e0,  -0.1803368742038817041144592e0,
    0.1602994490557515160717379e0,  -0.1442697693365754358167697e0,
    0.1311543594355372121787466e0,  -0.1202176960457003410683978e0,
    0.1109638171637819710470537e0,  -0.1031531003506912378084840e0,
    0.9645974908214134832482076e-1, -0.8935850319235346795703247e-1,
    0.8162444687576600431277924e-1, -0.8193119798697735675863112e-1,
    0.9425707890209205614610323e-1, -0.8667287589180527455416767e-1,
    0.3774234507330904507125737e-1};

template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    return x * abacus::internal::horner_polynomial(x, polynomialD);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T log2(const T x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type IntType;
  typedef FPShape<T> Shape;

  IntType exponent;

  T significand = __abacus_frexp(x, &exponent);

  const T sqrt1Over2 = 7.07106769084930419921875e-1;

  const SignedType cond1 = significand < sqrt1Over2;

  // By explicitly assigning.
  const T significandMultipliedByTwo = significand * 2.0f;
  significand = __abacus_select(significand, significandMultipliedByTwo, cond1);
  exponent = __abacus_select(exponent, exponent - 1,
                             abacus::detail::cast::convert<IntType>(cond1));

  significand = significand - 1.0f;

  T result = helper<T>::_(significand);

  result += abacus::detail::cast::convert<T>(exponent);

  const SignedType inf_as_int = Shape::ExponentMask();
  const T inf_for_type = abacus::detail::cast::as<T>(inf_as_int);

  const SignedType cond2 = x == static_cast<T>(0.0f);
  result = __abacus_select(result, -inf_for_type, cond2);

  const SignedType cond3 = __abacus_isinf(x);
  result = __abacus_select(result, inf_for_type, cond3);

  const SignedType cond4 = (x < static_cast<T>(0.0f)) | __abacus_isnan(x);
  return __abacus_select(result, FPShape<T>::NaN(), cond4);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_log2(abacus_half x) { return log2<>(x); }
abacus_half2 ABACUS_API __abacus_log2(abacus_half2 x) { return log2<>(x); }
abacus_half3 ABACUS_API __abacus_log2(abacus_half3 x) { return log2<>(x); }
abacus_half4 ABACUS_API __abacus_log2(abacus_half4 x) { return log2<>(x); }
abacus_half8 ABACUS_API __abacus_log2(abacus_half8 x) { return log2<>(x); }
abacus_half16 ABACUS_API __abacus_log2(abacus_half16 x) { return log2<>(x); }
#endif

abacus_float ABACUS_API __abacus_log2(abacus_float x) { return log2<>(x); }
abacus_float2 ABACUS_API __abacus_log2(abacus_float2 x) { return log2<>(x); }
abacus_float3 ABACUS_API __abacus_log2(abacus_float3 x) { return log2<>(x); }
abacus_float4 ABACUS_API __abacus_log2(abacus_float4 x) { return log2<>(x); }
abacus_float8 ABACUS_API __abacus_log2(abacus_float8 x) { return log2<>(x); }
abacus_float16 ABACUS_API __abacus_log2(abacus_float16 x) { return log2<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_log2(abacus_double x) { return log2<>(x); }
abacus_double2 ABACUS_API __abacus_log2(abacus_double2 x) { return log2<>(x); }
abacus_double3 ABACUS_API __abacus_log2(abacus_double3 x) { return log2<>(x); }
abacus_double4 ABACUS_API __abacus_log2(abacus_double4 x) { return log2<>(x); }
abacus_double8 ABACUS_API __abacus_log2(abacus_double8 x) { return log2<>(x); }
abacus_double16 ABACUS_API __abacus_log2(abacus_double16 x) {
  return log2<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
