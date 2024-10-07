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

#include <abacus/abacus_cast.h>
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
static ABACUS_CONSTANT abacus_half __codeplay_log1p_coeff_halfH1[8] = {
    1.0f16,
    -0.5f16,
    0.333251953125f16,
    -0.2486572265625f16,
    0.2005615234375f16,
    -0.1859130859375f16,
    0.162353515625f16,
    -7.257080078125e-2f16};

static ABACUS_CONSTANT abacus_half __codeplay_log1p_coeff_halfH2[9] = {
    1.0f16,
    -0.5f16,
    0.333251953125f16,
    -0.25f16,
    0.202392578125f16,
    -0.1673583984375f16,
    0.1256103515625f16,
    -0.1241455078125f16,
    0.12432861328125f16};

template <>
struct helper<abacus_half, abacus_half> {
  static abacus_half _(const abacus_half x) {
    // Check for special cases: +/-ABACUS_INFINITY, ABACUS_NAN, negative numbers
    if (__abacus_isnan(x) || x < -1.0f16) {
      return ABACUS_NAN_H;
    }

    if (__abacus_isinf(x)) {
      return (abacus_half)ABACUS_INFINITY;
    }
    if (x == -1.0f16) {
      return (abacus_half)-ABACUS_INFINITY;
    }

    // Special case for 1.6708984375 (0x3eaf), which has a ULP error of 2.01428.
    if (x == 1.6708984375f16) {
      return 0.982421875f16;
    }

    /*
            //--------------------------------------------
            //If denormals arn't supported in hardware but you want the correct
       result:
            if (abacus::detail::cast::as<unsigned short>(x) < (unsigned
       short)0x1801)
                    return x;

            if (abacus::detail::cast::as<unsigned short>(x) >= 0x8000 &&
       abacus::detail::cast::as<unsigned short>(x) < (unsigned short)0x99a7)
                    return x;
            //--------------------------------------------
    */

    // Get rid of values we can't just calculate log(1+x) on
    if (-0.4f16 < x && x < 0.7f16) {
      // p = fpminimax(log1p(x), [|1,2,3,4,5,6,7,8|],[|11...|],[-0.4;0.7], 0,
      // floating, relative);
      abacus_half result =
          abacus::internal::horner_polynomial(x, __codeplay_log1p_coeff_halfH1);
      return x * result;
    }

    abacus_int exponent = 0;
    abacus_half significand = __abacus_frexp(x + 1.0f16, &exponent);

    // Scale the significand in order to fit in the domain of the polynomial
    // approximation
    if (significand < ABACUS_SQRT1_2_H) {
      significand *= 2.0f16;
      exponent--;
    }
    // We are using the polynomial approximation of x+1 so we need to reduce
    // input by one.
    significand = significand - 1.0f16;

    // Polynomial approximating the function log(x+1)/x over the range
    // (1/sqrt(2))-1, sqrt(2)-1.
    // p = fpminimax(log(x + 1)/x, [|0,1,2,3,4,5,6,7,8|],[|11...|],[sqrt(0.5) -
    // 1, sqrt(2) - 1], floating, relative);
    //
    // To ensure extra accuracy around significand ~ 0, we require the generated
    // polynomial to have a constant term = 0.0, otherwise as significand -> 0
    // this constant term would take over and give an infinite ulp error.
    // In other words if
    // log(x + 1) ~ poly_approx = a0 + a1*x + a2*x^2 + a3*x^3 + .....
    // then we need a0 to be 0.
    // To ensure this we actually approximate:
    // log(x + 1)/x ~ poly_approx = a0 + a1*x + a2*x^2 + a3*x^3 + ....
    // and multiply by x: -->
    // log(x + 1) ~ x(a0 + a1*x + a2*x^2 + ...) = a0*x + a1*x^2 + a2*x^3 + ...
    // aka a poynomial with no constant term.

    abacus_half poly_approx = abacus::internal::horner_polynomial(
        significand, __codeplay_log1p_coeff_halfH2);

    abacus_half result = poly_approx * significand;

    const abacus_half fexponent = (abacus_half)exponent;

    return result + (fexponent * ABACUS_LN2_H);
  }
};

// Vectorized version:
template <typename T>
struct helper<T, abacus_half> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    typedef typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
        IntVecType;

    IntVecType exponent = 0;
    T significand = __abacus_frexp(x + 1.0f16, &exponent);

    SignedType exponent_short =
        abacus::detail::cast::convert<SignedType>(exponent);
    // Scale the significand in order to fit in the domain of the polynomial
    // approximation
    SignedType cond = significand < ABACUS_SQRT1_2_H;

    significand = __abacus_select(significand, significand * 2.0f16, cond);
    exponent_short = __abacus_select(exponent_short, exponent_short - 1, cond);

    // We are using the polynomial approximation of x+1 so we need to reduce
    // input by one.
    significand = significand - 1.0f16;

    T result = significand * abacus::internal::horner_polynomial(
                                 significand, __codeplay_log1p_coeff_halfH2);

    result = result +
             (abacus::detail::cast::convert<T>(exponent_short) * ABACUS_LN2_H);

    const T approx_threshold_1 = 0.7f16;
    const T approx_threshold_2 = -0.4f16;

    result =
        __abacus_select(result,
                        x * abacus::internal::horner_polynomial(
                                x, __codeplay_log1p_coeff_halfH1),
                        (approx_threshold_2 < x) && (x < approx_threshold_1));

    result = __abacus_select(result, __abacus_copysign(ABACUS_INFINITY, x),
                             (x == -1.0f16) | __abacus_isinf(x));

    result = __abacus_select(result, FPShape<T>::NaN(),
                             (x < -1.0f16) | __abacus_isnan(x));  // nan

    // Special case for 1.6708984375 (0x3eaf), which has a ULP error of 2.01428.
    result =
        __abacus_select(result, T(0.982421875f16), x == T(1.6708984375f16));

    return result;
  }
};
#endif  //__CA_BUILTINS_HALF_SUPPORT

// see maple worksheet for polynomial derivation
static ABACUS_CONSTANT abacus_float __codeplay_log1p_coeff[30] = {
    -0.5f,
    +0.333333126f,
    -0.250000096f,
    +0.200021187f,
    -0.166679959f,
    +0.142195524f,
    -0.124055888f,
    +0.118881618f,
    -0.116756522f,
    +0.0674664199f,
    0.0f,
    1.000000001f,
    -.4999994787f,
    .3333676883f,
    -.2491350578f,
    .2107867879f,
    -0.924008276e-1f,
    .4344614353f,
    .5044967528f,
    .7407703840f,
    0.0f,
    .9999999985f,
    -.4999995932f,
    .3333159994f,
    -.2497120596f,
    .1975572438f,
    -.1546546606f,
    .1061853047f,
    -0.5233163279e-1f,
    0.1288876423e-1f};

template <>
struct helper<abacus_float, abacus_float> {
  static abacus_float _(const abacus_float x) {
    // Check for special cases: +/-ABACUS_INFINITY, ABACUS_NAN, negative numbers
    if (__abacus_isnan(x) || x < -1.0f) {
      return ABACUS_NAN;
    } else if (x == -1.0f || __abacus_isinf(x)) {
      return __abacus_copysign(ABACUS_INFINITY, x);
    }

    abacus_int exponent = 0;

    abacus_float significand = __abacus_frexp(x + 1.0f, &exponent);

    // Scale the significand in order to fit in the domain of the polynomial
    // approximation
    if (significand < ABACUS_SQRT1_2_F) {
      significand *= 2.0f;
      exponent--;
    }
    // We are using the polynomial approximation of x+1 so we need to reduce
    // input by one.
    significand = significand - 1.0f;

    abacus_int polynomial_select = 0;

    const abacus_float approx_threshold_1 =
        __abacus_as_float(0x3f2610c3);  // 6.48693263530731201171875E-1
    if (x <= approx_threshold_1 && x >= 0) {
      polynomial_select = 2;
      significand = x;
    }

    const abacus_float approx_threshold_2 =
        __abacus_as_float(0xbec974cf);  //-3.934693038463592529296875E-1
    if (x < 0 && x >= approx_threshold_2) {
      significand = x;
      polynomial_select = 1;
    }

    const abacus_float poly_approx = abacus::internal::horner_polynomial(
        significand, __codeplay_log1p_coeff + (polynomial_select * (size_t)10),
        10);

    if (polynomial_select != 0) {
      return poly_approx;
    }

    const abacus_float result =
        significand + (significand * significand * poly_approx);

    const abacus_float fexponent = (abacus_float)(exponent);

    return result + (fexponent * ABACUS_LN2_F);
  }
};

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    SignedType exponent = 0;
    T significand = __abacus_frexp(x + 1.0f, &exponent);

    // Scale the significand in order to fit in the domain of the polynomial
    // approximation
    const SignedType cond = significand < ABACUS_SQRT1_2_F;

    significand = __abacus_select(significand, significand * 2.0f, cond);
    exponent = __abacus_select(exponent, exponent - 1, cond);

    // We are using the polynomial approximation of x+1 so we need to reduce
    // input by one.
    significand = significand - 1.0f;

    T result = abacus::internal::horner_polynomial(significand,
                                                   __codeplay_log1p_coeff, 10);

    result = significand + significand * significand * result;

    result =
        result + (abacus::detail::cast::convert<T>(exponent) * ABACUS_LN2_F);

    const T approx_threshold_1 =
        __abacus_as_float(0x3f2610c3);  // 6.48693263530731201171875E-1

    result = __abacus_select(
        result,
        abacus::internal::horner_polynomial(x, __codeplay_log1p_coeff + 20, 10),
        (x <= approx_threshold_1) & (x >= 0));

    const T approx_threshold_2 =
        __abacus_as_float(0xbec974cf);  //-3.934693038463592529296875E-1

    result = __abacus_select(
        result,
        abacus::internal::horner_polynomial(x, __codeplay_log1p_coeff + 10, 10),
        (x < 0) & (x >= approx_threshold_2));

    result = __abacus_select(result, __abacus_copysign(ABACUS_INFINITY, x),
                             (x == -1.0f) | __abacus_isinf(x));
    result =
        __abacus_select(result, ABACUS_NAN, (x < -1.0f) | __abacus_isnan(x));

    return result;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
static ABACUS_CONSTANT abacus_double polynomialD1[21] = {
    0.1000000000000000006635833e1, -0.4999999999999884526147957e0,
    0.3333333333366707015642681e0, -0.2499999996173379046025414e0,
    0.2000000231996332007968923e0, -0.1666658072284573487778084e0,
    0.1428783466418025098218963e0, -0.1246315203921042573031084e0,
    0.115797593824177480108261e0,  -0.55195690712225414708243e-1,
    0.418901741730404513048826e0,  0.1777680688245529327744106e1,
    0.8316530989625519023089370e1, 0.2844323860406127679112277e2,
    0.7691830696215821989851716e2, 0.1596425806469532131609827e3,
    0.2512403822731564794013818e3, 0.2895588507836496816241960e3,
    0.2316034378476612831740669e3, 0.1151851238135561777212998e3,
    0.2719992270243911326603090e2};

static ABACUS_CONSTANT abacus_double polynomialD2[24] = {
    0.9999999999999999999595219e0,  -0.4999999999999999526366799e0,
    0.3333333333333240961069276e0,  -0.2499999999992816242549627e0,
    0.1999999999702594769577943e0,  -0.1666666659078358611846577e0,
    0.1428571298238767208511871e0,  -0.1249998402354328681705336e0,
    0.1111096545177450472688009e0,  -0.9998981889873864015067643e-1,
    0.9085326557872509105369545e-1, -0.8308887462132954652641194e-1,
    0.7605591436663234408797520e-1, -0.6890732679896749090126449e-1,
    0.6059659793937339284584698e-1, -0.5027976806608310668600134e-1,
    0.3803293430301725192579005e-1, -0.2527095333710411410042101e-1,
    0.1419365724050995506533252e-1, -0.6465358551074126224037865e-2,
    0.2272206596491669714665425e-2, -0.5742644845442336251095989e-3,
    0.9245695423113694038790722e-4, -0.7099163849308927994024592e-5};

static ABACUS_CONSTANT abacus_double polynomialD3[16] = {
    0.6931471805599453211688513e0,   0.4999999999999936116652162e0,
    -0.1249999999994259924381910e0,  0.4166666664633423887374388e-1,
    -0.1562499962227076988443045e-1, 0.6249995755793789780857597e-2,
    -0.2604135305086186661622359e-2, 0.1115910678723805392821639e-2,
    -0.4876891681108076205809686e-3, 0.2154095918242489278252579e-3,
    -0.9440935271876453434287800e-4, 0.3943424269566617128001326e-4,
    -0.1460752166319748750839019e-4, 0.4311915376956372242612062e-5,
    -0.8657183573415959274002373e-6, 0.8596505513189088569636363e-7};

template <>
struct helper<abacus_double, abacus_double> {
  static abacus_double _(abacus_double x) {
    if (-0.5 <= x && x < 0.0) {
      return x * abacus::internal::horner_polynomial(x, polynomialD1);
    } else if (0.0 <= x && x < 1.0) {
      return x * abacus::internal::horner_polynomial(x, polynomialD2);
    } else if (1.0 <= x && x < 2.0) {
      return abacus::internal::horner_polynomial(x - 1.0, polynomialD3);
    } else {
      return __abacus_log(x + 1.0);
    }
  }
};

template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    T result = __abacus_log(x + 1.0);

    const SignedType cond1 = (x >= -0.5) & (x < 0.0);
    result = __abacus_select(
        result, x * abacus::internal::horner_polynomial(x, polynomialD1),
        cond1);

    const SignedType cond2 = (x >= 0.0) & (x < 1.0);
    result = __abacus_select(
        result, x * abacus::internal::horner_polynomial(x, polynomialD2),
        cond2);

    const SignedType cond3 = (x >= 1.0) & (x < 2.0);
    result = __abacus_select(
        result, abacus::internal::horner_polynomial(x - 1.0, polynomialD3),
        cond3);

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
}  // namespace

namespace {
template <typename T>
T log1p(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT

abacus_half ABACUS_API __abacus_log1p(abacus_half x) { return log1p<>(x); }
abacus_half2 ABACUS_API __abacus_log1p(abacus_half2 x) { return log1p<>(x); }
abacus_half3 ABACUS_API __abacus_log1p(abacus_half3 x) { return log1p<>(x); }
abacus_half4 ABACUS_API __abacus_log1p(abacus_half4 x) { return log1p<>(x); }
abacus_half8 ABACUS_API __abacus_log1p(abacus_half8 x) { return log1p<>(x); }
abacus_half16 ABACUS_API __abacus_log1p(abacus_half16 x) { return log1p<>(x); }

#endif  //__CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_log1p(abacus_float x) { return log1p<>(x); }
abacus_float2 ABACUS_API __abacus_log1p(abacus_float2 x) { return log1p<>(x); }
abacus_float3 ABACUS_API __abacus_log1p(abacus_float3 x) { return log1p<>(x); }
abacus_float4 ABACUS_API __abacus_log1p(abacus_float4 x) { return log1p<>(x); }
abacus_float8 ABACUS_API __abacus_log1p(abacus_float8 x) { return log1p<>(x); }
abacus_float16 ABACUS_API __abacus_log1p(abacus_float16 x) {
  return log1p<>(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_log1p(abacus_double x) { return log1p<>(x); }
abacus_double2 ABACUS_API __abacus_log1p(abacus_double2 x) {
  return log1p<>(x);
}
abacus_double3 ABACUS_API __abacus_log1p(abacus_double3 x) {
  return log1p<>(x);
}
abacus_double4 ABACUS_API __abacus_log1p(abacus_double4 x) {
  return log1p<>(x);
}
abacus_double8 ABACUS_API __abacus_log1p(abacus_double8 x) {
  return log1p<>(x);
}
abacus_double16 ABACUS_API __abacus_log1p(abacus_double16 x) {
  return log1p<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
