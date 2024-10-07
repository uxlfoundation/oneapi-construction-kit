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
#include <abacus/internal/add_exact.h>
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/multiply_exact.h>
#include <abacus/internal/payne_hanek.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

template <typename T>
struct helper<T, abacus_float> {
  static T numerator(const T x) {
    return (x * 0.999999986f) - (x * x * x * 0.0958010197f);
  }

  static T denominator(const T x) {
    return (T)1.f - (x * x * 0.429135022f) + (x * x * x * x * 0.00971659383f);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T numerator(const T x) {
    const T xSq = x * x;
    // see maple worksheet for polynomial derivation
    const abacus_double polynomial[5] = {
        -0.80733427364182778996719723073260349e-1,
        0.16315384900944236013062161513423742e-2,
        -0.10581062310830882213842634271414963e-4,
        2.0758861431786944948715354363504356e-8,
        -5.9926098258796618469322956805282730e-12};

    const T y = abacus::internal::horner_polynomial(xSq, polynomial);
    return (x * .78539816339744830961566084582040377) +
           (x * xSq * .85737772729049709971879473936502462 * y);
  }

  static T denominator(const T x) {
    const T xSq = x * x;

    const abacus_double polynomial[5] = {
        -.34261349592750523798202965484003723,
        0.13351078274661912557058825219180004e-1,
        -0.15080878516011152859824743647532211e-3,
        5.5444607846133240887999568049688666e-7,
        -5.0244514118826496404940417101797213e-10};

    const T y = abacus::internal::horner_polynomial(xSq, polynomial);
    return (xSq * y * .85737772729049709971879473936502462) + 1.0;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T tan(const T x) {
  using SignedType = typename TypeTraits<T>::SignedType;
  using IntType =
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type;

  IntType octet = 0;
  // Range reduction from 0 -> pi/4:
  const T xReduced = abacus::internal::payne_hanek(x, &octet);

  // we calculate both cos_approx and sin_approx anyway, so use sincos approx:

  const T tan_numerator = helper<T>::numerator(xReduced);
  const T tan_denominator = helper<T>::denominator(xReduced);

  const SignedType cond1 =
      abacus::detail::cast::convert<SignedType>(((octet + 1) & 0x2) == 0);
  const T y = __abacus_select(tan_denominator, tan_numerator, cond1) /
              __abacus_select(tan_numerator, tan_denominator, cond1);

  // sign changes depending on octet:
  const SignedType cond2 =
      abacus::detail::cast::convert<SignedType>((octet & 0x2) == 0);
  return __abacus_select(-y, y, cond2);
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
// see tan sollya script for derivations
static ABACUS_CONSTANT abacus_half _tan1H[5] = {
    0.78564453125f16, 0.161376953125f16, 4.0740966796875e-2f16,
    7.549285888671875e-3f16, 4.9285888671875e-3f16};
static ABACUS_CONSTANT abacus_half _tan2H[5] = {
    1.2734375f16, -0.264404296875f16, -1.54209136962890625e-3f16,
    -1.275634765625e-2f16, 5.283355712890625e-3f16};

template <typename T>
T tan_half(const T x) {
  using SignedType = typename TypeTraits<T>::SignedType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;

  SignedType octant = 0;

  // Range reduction from 0 -> pi/4.
  // Note that the returned value is actually x / (pi/4), which improves the
  // accuracy of the polynomials.
  const T xReduced = abacus::internal::payne_hanek_half(x, &octant);

  // Depending on the octant (1/8th of the unit circle), we need to calculate
  // either tan(x) or cot(x). We know that tan(x) repeats over a 2pi interval.
  // We also know that payne_hanek_half will reduce the value of x to be modulo
  // pi/4 (between 0 and pi/4). More interestingly, if x modulo pi/2 would be
  // between pi/4 and pi/2, then payne_hanek_half will actually return pi/4 - (x
  // modulo pi/4) (to improve accuracy).
  //
  // Taking these facts into account, we can divide the unit circle into 8
  // octants (numbered 0 to 7):
  /*
              pi/2
               +
           \ 2 | 1 /
            \ X|X /
          3 X\ | /X 0
           X  \|/  X
     pi  +-----+-------+ 0 or 2pi
           X  /|\  X
          4 X/ | \X 7
            / X|X \
           / 5 | 6 \
               +
             3pi/2
  */
  // To calculate the value of tan given the reduced value of x (which we call
  // xReduced) and the octant, we need consider each octant separately and
  // make use of various trig identities.
  //
  // We know that tan has a period of pi, so we can assume octants 4-7 are
  // equivalent to 0-3. Lets consider each octant in turn.
  //
  // Octant 0 (x in [0 .. pi/4]):
  //  xReduced = x
  //  tan(x) = tan(xReduced)            substitute x
  //
  // Octant 1 (x in [pi/4 .. pi/2]):
  //  xReduced = pi/4 - (x - pi/4)
  //           = pi/2 - x
  //  x = pi/2 - xReduced
  //  tan(x) = tan(pi/2 - xReduced)     substitute x
  //         = cot(xReduced)            use identity: cot(x) = tan(pi/2 - x)
  //
  // Octant 2 (x in [pi/2 .. 3pi/4]):
  //  xReduced = x - pi/2
  //  x = xReduced + pi/2
  //  tan(x) = tan(xReduced + pi/2)     substitute x
  //         = -cot(xReduced)           use identity: tan(x + pi/2) = -cot(x)
  //
  // Octant 3 (x in [3pi/4 .. pi]):
  //  xReduced = pi/4 - (x - 3pi/4)
  //           = pi - x
  //  x = pi - xReduced
  //  tan(x) = tan(pi - xReduced)       substitute x
  //         = -tan(xReduced)           use identity: tan(pi - x) = -tan(x)

  // Many of the polynomials below are based on xReduced^2, rather than
  // xReduced.
  const T x2 = xReduced * xReduced;

  // It doesn't seem we can estimate tan(x) to the required accuracy with just a
  // normal polynomial. To this end, rather than evaluating in a higher
  // precision, we can just make the last coefficient of the polynomial extra
  // precise. This helps with cancellations in the polynomial that occur.
  //
  // So instead of your normal 16bit polynomial:  a + b*x + c*x^2 + d*x^3 + ....
  // With a,b,c,d all 16 bit, we instead let 'a' have more precision. We let it
  // have 22 bits of mantissa precision, instead of the normal 11 for half.
  // This has the nice property that 'a' can now be split into the sum of 2
  // halfs: a_hi and a_lo.
  //
  // So now our polynomial is:   a_lo + (a_hi + b*x + c*x^2 + d*x^3 + ....),
  // where it's important we add a_lo at the end. This basically gives more
  // precision for just one extra add.
  // The 'extra_precision_terms' below are the 'a_lo' in this case, they are
  // generated by getting the 22-bit 'a' from sollya, and subtracting off the
  // 16-bit 'a_hi' = (half closest to a).

  // Calculate tan(xReduced):

  // a = 0.7854006290435791015625
  //   a_hi = 0.78564453125 (part of _tan1H)
  //   a_lo = -2.4390220642089844e-4f16
  const T poly1_extra_precision_term1 = -2.4390220642089844e-4f16;

  T poly_add_lo = 0;
  const T poly_add_hi = abacus::internal::add_exact(
      abacus::internal::horner_polynomial(x2, _tan1H),
      poly1_extra_precision_term1, &poly_add_lo);

  T tanX = 0;

  // On systems with missing denormal support, a FTZ happens with some inputs
  // during the multiply_exact operation. To avoid this, we scale xReduced by
  // 2^9, then multiply the result by 2^-9 to reverse the scaling.
  if (__abacus_isftz()) {
    const T ftz_multiplier = 512.0f16;
    const T inv_ftz_multiplier = 0.001953125f16;
    const T scaled_xReduced = xReduced * ftz_multiplier;

    T poly_add_mul_lo = 0;
    const T poly_add_mul_hi = abacus::internal::multiply_exact(
        poly_add_hi, scaled_xReduced, &poly_add_mul_lo);
    poly_add_mul_lo += poly_add_lo * scaled_xReduced;

    tanX = (poly_add_mul_hi + poly_add_mul_lo) * inv_ftz_multiplier;
  } else {
    T poly_add_mul_lo = 0;
    const T poly_add_mul_hi = abacus::internal::multiply_exact(
        poly_add_hi, xReduced, &poly_add_mul_lo);
    poly_add_mul_lo += poly_add_lo * xReduced;

    tanX = poly_add_mul_hi + poly_add_mul_lo;
  }

  // Calculate cot(xReduced):
  const T cotX = abacus::internal::horner_polynomial(x2, _tan2H) / xReduced;

  // Select either tan(xReduced) or cot(xReduced) depending on the section of
  // the unit circle that x resides in. tan has a period of 'pi', so we only
  // care about the last 3 bits of the octant.
  octant = (octant & 0x3);
  T ans = __abacus_select(cotX, tanX, SignedType(((octant + 1) & 0x3) < 2));

  // We need a single value fix, otherwise this is a 2.085 ULP error:
  // Value is at x = +-90.0
  const T xAbs = __abacus_fabs(x);
  ans = __abacus_select(ans, abacus::detail::cast::as<T>(UnsignedType(0x3ffb)),
                        SignedType(xAbs == 90.0f16));

  if (__abacus_isftz()) {
    // If x is +- 532.5 then xReduced returned by
    // `abacus::internal::payne_hanek_half` has an exponent of -15, aka
    // denormal. However the final result from `tan()` is normal so we can't
    // return 0 as a FTZ result when denormals aren't supported. Instead
    // hardcode the result for this specific input case.
    ans =
        __abacus_select(ans, abacus::detail::cast::as<T>(UnsignedType(0x7566)),
                        SignedType(xAbs == 532.5f16));
  }

  // Sign changes depending on octant.
  const SignedType cond2 = (octant >= 2) ^ (x < 0.0f16);
  ans = __abacus_select(ans, -ans, cond2);

  return ans;
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_tan(abacus_half x) { return tan_half(x); }
abacus_half2 ABACUS_API __abacus_tan(abacus_half2 x) { return tan_half(x); }
abacus_half3 ABACUS_API __abacus_tan(abacus_half3 x) { return tan_half(x); }
abacus_half4 ABACUS_API __abacus_tan(abacus_half4 x) { return tan_half(x); }
abacus_half8 ABACUS_API __abacus_tan(abacus_half8 x) { return tan_half(x); }
abacus_half16 ABACUS_API __abacus_tan(abacus_half16 x) { return tan_half(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_tan(abacus_float x) { return tan<>(x); }
abacus_float2 ABACUS_API __abacus_tan(abacus_float2 x) { return tan<>(x); }
abacus_float3 ABACUS_API __abacus_tan(abacus_float3 x) { return tan<>(x); }
abacus_float4 ABACUS_API __abacus_tan(abacus_float4 x) { return tan<>(x); }
abacus_float8 ABACUS_API __abacus_tan(abacus_float8 x) { return tan<>(x); }
abacus_float16 ABACUS_API __abacus_tan(abacus_float16 x) { return tan<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_tan(abacus_double x) { return tan<>(x); }
abacus_double2 ABACUS_API __abacus_tan(abacus_double2 x) { return tan<>(x); }
abacus_double3 ABACUS_API __abacus_tan(abacus_double3 x) { return tan<>(x); }
abacus_double4 ABACUS_API __abacus_tan(abacus_double4 x) { return tan<>(x); }
abacus_double8 ABACUS_API __abacus_tan(abacus_double8 x) { return tan<>(x); }
abacus_double16 ABACUS_API __abacus_tan(abacus_double16 x) { return tan<>(x); }
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
