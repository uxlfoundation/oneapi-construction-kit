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
#include <abacus/internal/floating_point.h>
#include <abacus/internal/fmod_unsafe.h>
#include <abacus/internal/is_denorm.h>

namespace {

// Describes how to scale denormal numbers depending on float precision.
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct scaleFactor;

#ifdef __CA_BUILTINS_HALF_SUPPORT
// xUint | lowExpBit     sets the exponent to -14
// 64                    2^6
// Multiplication        exponent = -14 + 6 = -8
//
// scaled value          (x * 2^6) + 2^-8 - 2^-8
//                       (x * 2^6)
template <typename T>
struct scaleFactor<T, abacus_half> {
  static constexpr abacus_float up = 64.0f16;            // 2^6
  static constexpr abacus_float down = 0.015625f16;      // 2^-6
  static constexpr abacus_float adjust = 0.00390625f16;  // 2^-8
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

// xUint | lowExpBit     sets the exponent to -126
// 16777216              2^24
// Multiplication        exponent = -126 + 24 = -102
//
// scaled value          (x * 2^24) + 2^-102 - 2^-102
//                       (x * 2^24)
template <typename T>
struct scaleFactor<T, abacus_float> {
  static constexpr abacus_float up = 16777216.0f;        // 2^24
  static constexpr abacus_float down = 5.9604645E-8;     // 2^-24
  static constexpr abacus_float adjust = 1.9721523E-31;  // 2^-102
};

// Scale denormal input 'x' up so that we can perform operations on 'x' without
// the hardware flushing to zero. Done by setting the least significant exponent
// bit to make 'x' a normal number before doing arithmetic to adjust for a
// precision dependent scaling factor.
template <typename T>
T upscaleDenormal(const T x) {
  using UnsignedType = typename TypeTraits<T>::UnsignedType;

  const UnsignedType lowExpBit = 0x1 << FPShape<T>::Mantissa();
  const UnsignedType xUint = abacus::detail::cast::as<UnsignedType>(x);
  const UnsignedType scale = xUint | lowExpBit;

  const T result = abacus::detail::cast::as<T>(scale) * scaleFactor<T>::up -
                   scaleFactor<T>::adjust;
  return result;
}

template <typename T>
T remquo_helper_scalar(const T x, const T m, abacus_int *out_quo) {
  static_assert(TypeTraits<T>::num_elements == 1,
                "Should only be called with scalar types");
  using SignedType = typename TypeTraits<T>::SignedType;

  if (!__abacus_isfinite(x) || __abacus_isnan(m) || (T(0) == m)) {
    *out_quo = 0;

    return FPShape<T>::NaN();
  }

  if (__abacus_isinf(m)) {
    *out_quo = 0;
    return x;
  }

  T mAbs = __abacus_fabs(m);

  abacus_int quotient = 0;
  T result = abacus::internal::fmod_unsafe(x, m, &quotient);

  // If 'result' is a denormal number and the architecture is FTZ then the
  // 'result2' bounds calculation will resolve to zero. Avoid this by scaling
  // 'result' and 'mAbs' up by our scale factor
  const bool scaleDenorm =
      abacus::internal::is_denorm(result) && __abacus_isftz();
  if (scaleDenorm) {
    result = upscaleDenormal<T>(result);

    // Scale 'mAbs' so it's in sync with 'result'
    if (abacus::internal::is_denorm(mAbs)) {
      mAbs = upscaleDenormal<T>(mAbs);
    } else {
      // 'mAbs' isn't a denormal, we can multiply directly by our scale factor
      mAbs *= scaleFactor<T>::up;
    }
  }

  // Checks for bounds and RTE
  const T result2 = result * (T)2.0;
  if ((result2 > mAbs) || (((quotient & 0x1) == 0x1) && (result2 == mAbs))) {
    // Mask increment with 0x7F since we need to return 7 bits of quotient,
    // which `fmod_unsafe()` already does for us, but increment operation might
    // overflow this bound.
    quotient = (quotient + 1) & 0x7F;
    result -= mAbs;
  }

  if (scaleDenorm) {
    // Since we scaled 'result' up to avoid FTZ, now scale it back down by the
    // same factor.
    result *= scaleFactor<T>::down;
  }

  *out_quo = quotient * __abacus_select(1, -1, __abacus_signbit(x * m));

  return __abacus_select(result, -result, (SignedType)(x < 0));
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <>
abacus_double remquo_helper_scalar(const abacus_double x, const abacus_double m,
                                   abacus_int *out_quo) {
  if (!__abacus_isfinite(x) || __abacus_isnan(x) || __abacus_isnan(m) ||
      (m == 0.0)) {
    *out_quo = 0;
    return ABACUS_NAN;
  }

  if (__abacus_isinf(m)) {
    *out_quo = 0;
    return x;
  }

  const abacus_double mAbs = __abacus_fabs(m);

  abacus_int quotient = 0;
  abacus_double result = abacus::internal::fmod_unsafe(x, m, &quotient);

  // Checks for bounds and RTE
  const abacus_double result2 = result * 2.0;
  if ((result2 > mAbs) || (((quotient & 0x1) == 0x1) && (result2 == mAbs))) {
    quotient++;
    result -= mAbs;
  }

  *out_quo =
      quotient * __abacus_select(1, -1, (abacus_int)__abacus_signbit(x * m));

  return __abacus_select(result, -result, (abacus_long)(x < 0.0));
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T, typename IntVecType,
          typename E = typename TypeTraits<T>::ElementType>
struct helper {
  static T remquo_impl(const T x, const T m, IntVecType *out_quo) {
    using SignedType = typename TypeTraits<T>::SignedType;
    using Shape = FPShape<T>;

    T mAbs = __abacus_fabs(m);

    IntVecType quotient = 0;
    T result = abacus::internal::fmod_unsafe(x, m, &quotient);

    // If 'result' is a denormal number and the architecture is FTZ then the
    // 'result2' bounds calculation will resolve to zero. Avoid this by scaling
    // 'result' and 'mAbs' up by scale factor
    const SignedType scaleDenorm = abacus::internal::is_denorm(result);
    if (__abacus_isftz()) {
      result = __abacus_select(result, upscaleDenormal<T>(result), scaleDenorm);

      // If 'mAbs' isn't a denormal, we can multiply directly by our scale
      // factor
      const T scaledmAbs =
          __abacus_select(mAbs * scaleFactor<T>::up, upscaleDenormal<T>(mAbs),
                          abacus::internal::is_denorm(mAbs));
      mAbs = __abacus_select(mAbs, scaledmAbs, scaleDenorm);
    }

    // Checks for bounds and RTE
    const T result2 = result * 2;
    const SignedType c2 =
        (result2 > mAbs) |
        (abacus::detail::cast::convert<SignedType>((quotient & 0x1) == 0x1) &
         (result2 == mAbs));

    result = __abacus_select(result, result - mAbs, c2);

    // Mask increment with 0x7F since we need to return 7 bits of quotient,
    // which `fmod_unsafe()` already does for us, but increment operation might
    // overflow this bound.
    quotient = __abacus_select(quotient, (quotient + 1) & 0x7F,
                               abacus::detail::cast::convert<IntVecType>(c2));

    quotient *= __abacus_select(
        (IntVecType)1, -1,
        abacus::detail::cast::convert<IntVecType>(__abacus_signbit(x * m)));

    if (__abacus_isftz()) {
      // If we scaled 'result' up to avoid FTZ, now scale it back down by the
      // same factor
      result =
          __abacus_select(result, result * scaleFactor<T>::down, scaleDenorm);
    }

    result = __abacus_select(result, -result, x < T(0));

    const SignedType c3 =
        ~__abacus_isfinite(x) | __abacus_isnan(m) | (m == T(0));

    *out_quo = __abacus_select(quotient, 0,
                               abacus::detail::cast::convert<IntVecType>(c3));

    return __abacus_select(result, Shape::NaN(), c3);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T, typename IntVecType>
struct helper<T, IntVecType, abacus_double> {
  static T remquo_impl(const T x, const T m, IntVecType *out_quo) {
    using SignedType = typename TypeTraits<T>::SignedType;

    T mAbs = __abacus_fabs(m);

    IntVecType quotient = 0;
    T result = abacus::internal::fmod_unsafe(x, m, &quotient);

    // Checks for bounds and RTE
    const T result2 = result * 2;
    const SignedType c2 =
        (result2 > mAbs) |
        (abacus::detail::cast::convert<SignedType>((quotient & 0x1) == 0x1) &
         (result2 == mAbs));

    result = __abacus_select(result, result - mAbs, c2);
    quotient = __abacus_select(quotient, quotient + 1,
                               abacus::detail::cast::convert<IntVecType>(c2));

    quotient *= __abacus_select(
        (IntVecType)1, -1,
        abacus::detail::cast::convert<IntVecType>(__abacus_signbit(x * m)));

    result = __abacus_select(result, -result, x < 0.0);

    const SignedType c3 =
        ~__abacus_isfinite(x) | __abacus_isnan(m) | (m == 0.0);

    *out_quo = __abacus_select(quotient, 0,
                               abacus::detail::cast::convert<IntVecType>(c3));
    return __abacus_select(result, (T)ABACUS_NAN, c3);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T remquo_helper_vector(
    const T x, const T m,
    typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type *o) {
  static_assert(TypeTraits<T>::num_elements != 1,
                "Should only be called with vector types");
  using IntVecType =
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type;
  return helper<T, IntVecType>::remquo_impl(x, m, o);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_remquo(abacus_half x, abacus_half m,
                                       abacus_int *o) {
  return remquo_helper_scalar(x, m, o);
}
abacus_half2 ABACUS_API __abacus_remquo(abacus_half2 x, abacus_half2 m,
                                        abacus_int2 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_half3 ABACUS_API __abacus_remquo(abacus_half3 x, abacus_half3 m,
                                        abacus_int3 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_half4 ABACUS_API __abacus_remquo(abacus_half4 x, abacus_half4 m,
                                        abacus_int4 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_half8 ABACUS_API __abacus_remquo(abacus_half8 x, abacus_half8 m,
                                        abacus_int8 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_half16 ABACUS_API __abacus_remquo(abacus_half16 x, abacus_half16 m,
                                         abacus_int16 *o) {
  return remquo_helper_vector(x, m, o);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_remquo(abacus_float x, abacus_float m,
                                        abacus_int *o) {
  return remquo_helper_scalar(x, m, o);
}
abacus_float2 ABACUS_API __abacus_remquo(abacus_float2 x, abacus_float2 m,
                                         abacus_int2 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_float3 ABACUS_API __abacus_remquo(abacus_float3 x, abacus_float3 m,
                                         abacus_int3 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_float4 ABACUS_API __abacus_remquo(abacus_float4 x, abacus_float4 m,
                                         abacus_int4 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_float8 ABACUS_API __abacus_remquo(abacus_float8 x, abacus_float8 m,
                                         abacus_int8 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_float16 ABACUS_API __abacus_remquo(abacus_float16 x, abacus_float16 m,
                                          abacus_int16 *o) {
  return remquo_helper_vector(x, m, o);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_remquo(abacus_double x, abacus_double m,
                                         abacus_int *o) {
  return remquo_helper_scalar(x, m, o);
}
abacus_double2 ABACUS_API __abacus_remquo(abacus_double2 x, abacus_double2 m,
                                          abacus_int2 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_double3 ABACUS_API __abacus_remquo(abacus_double3 x, abacus_double3 m,
                                          abacus_int3 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_double4 ABACUS_API __abacus_remquo(abacus_double4 x, abacus_double4 m,
                                          abacus_int4 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_double8 ABACUS_API __abacus_remquo(abacus_double8 x, abacus_double8 m,
                                          abacus_int8 *o) {
  return remquo_helper_vector(x, m, o);
}
abacus_double16 ABACUS_API __abacus_remquo(abacus_double16 x, abacus_double16 m,
                                           abacus_int16 *o) {
  return remquo_helper_vector(x, m, o);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
