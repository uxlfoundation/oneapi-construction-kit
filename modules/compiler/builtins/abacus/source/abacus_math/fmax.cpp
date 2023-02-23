// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T, typename U>
T fmax(const T x, const U y);

template <typename T>
T fmax(const T x, const T y) {
  typedef typename TypeTraits<T>::SignedType TSignedType;

  const TSignedType xNan = __abacus_isnan(x);
  const TSignedType yNan = __abacus_isnan(y);

  TSignedType c = 0;  // Return y when set

  if (__abacus_isftz()) {
    const TSignedType xInt = abacus::detail::cast::as<TSignedType>(x);
    const TSignedType yInt = abacus::detail::cast::as<TSignedType>(y);

    // (xInt < 0) & (yInt >= 0) - Then the sign bit of x is set, and sign
    // bit of y isn't. So x is negative and y is positive, set to true to
    // return y.
    //
    // (xInt >= yInt) - If integer representation xInt is larger than yInt
    // then float x will also be larger than float y. However this only
    // holds if both x and y are positive, due to 2s complement representation
    // of negative integers, so XOR to get the other cases where x will be
    // smaller.
    c = ((xInt >= yInt) ^ (xInt >= 0)) | ((xInt < 0) & (yInt >= 0));
  } else {
    c = x < y;
  }
  const TSignedType condition = (xNan | c) & ~yNan;
  return __abacus_select(x, y, condition);
}

template <typename T, typename U>
T fmax(const T x, const U y) {
  return fmax<>(x, (T)y);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_fmax(abacus_half x, abacus_half y) {
  return fmax<>(x, y);
}
abacus_half2 ABACUS_API __abacus_fmax(abacus_half2 x, abacus_half2 y) {
  return fmax<>(x, y);
}
abacus_half3 ABACUS_API __abacus_fmax(abacus_half3 x, abacus_half3 y) {
  return fmax<>(x, y);
}
abacus_half4 ABACUS_API __abacus_fmax(abacus_half4 x, abacus_half4 y) {
  return fmax<>(x, y);
}
abacus_half8 ABACUS_API __abacus_fmax(abacus_half8 x, abacus_half8 y) {
  return fmax<>(x, y);
}
abacus_half16 ABACUS_API __abacus_fmax(abacus_half16 x, abacus_half16 y) {
  return fmax<>(x, y);
}

abacus_half2 ABACUS_API __abacus_fmax(abacus_half2 x, abacus_half y) {
  return fmax<>(x, y);
}
abacus_half3 ABACUS_API __abacus_fmax(abacus_half3 x, abacus_half y) {
  return fmax<>(x, y);
}
abacus_half4 ABACUS_API __abacus_fmax(abacus_half4 x, abacus_half y) {
  return fmax<>(x, y);
}
abacus_half8 ABACUS_API __abacus_fmax(abacus_half8 x, abacus_half y) {
  return fmax<>(x, y);
}
abacus_half16 ABACUS_API __abacus_fmax(abacus_half16 x, abacus_half y) {
  return fmax<>(x, y);
}

#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_fmax(abacus_float x, abacus_float y) {
  return fmax<>(x, y);
}
abacus_float2 ABACUS_API __abacus_fmax(abacus_float2 x, abacus_float2 y) {
  return fmax<>(x, y);
}
abacus_float3 ABACUS_API __abacus_fmax(abacus_float3 x, abacus_float3 y) {
  return fmax<>(x, y);
}
abacus_float4 ABACUS_API __abacus_fmax(abacus_float4 x, abacus_float4 y) {
  return fmax<>(x, y);
}
abacus_float8 ABACUS_API __abacus_fmax(abacus_float8 x, abacus_float8 y) {
  return fmax<>(x, y);
}
abacus_float16 ABACUS_API __abacus_fmax(abacus_float16 x, abacus_float16 y) {
  return fmax<>(x, y);
}

abacus_float2 ABACUS_API __abacus_fmax(abacus_float2 x, abacus_float y) {
  return fmax<>(x, y);
}
abacus_float3 ABACUS_API __abacus_fmax(abacus_float3 x, abacus_float y) {
  return fmax<>(x, y);
}
abacus_float4 ABACUS_API __abacus_fmax(abacus_float4 x, abacus_float y) {
  return fmax<>(x, y);
}
abacus_float8 ABACUS_API __abacus_fmax(abacus_float8 x, abacus_float y) {
  return fmax<>(x, y);
}
abacus_float16 ABACUS_API __abacus_fmax(abacus_float16 x, abacus_float y) {
  return fmax<>(x, y);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_fmax(abacus_double x, abacus_double y) {
  return fmax<>(x, y);
}
abacus_double2 ABACUS_API __abacus_fmax(abacus_double2 x, abacus_double2 y) {
  return fmax<>(x, y);
}
abacus_double3 ABACUS_API __abacus_fmax(abacus_double3 x, abacus_double3 y) {
  return fmax<>(x, y);
}
abacus_double4 ABACUS_API __abacus_fmax(abacus_double4 x, abacus_double4 y) {
  return fmax<>(x, y);
}
abacus_double8 ABACUS_API __abacus_fmax(abacus_double8 x, abacus_double8 y) {
  return fmax<>(x, y);
}
abacus_double16 ABACUS_API __abacus_fmax(abacus_double16 x, abacus_double16 y) {
  return fmax<>(x, y);
}

abacus_double2 ABACUS_API __abacus_fmax(abacus_double2 x, abacus_double y) {
  return fmax<>(x, y);
}
abacus_double3 ABACUS_API __abacus_fmax(abacus_double3 x, abacus_double y) {
  return fmax<>(x, y);
}
abacus_double4 ABACUS_API __abacus_fmax(abacus_double4 x, abacus_double y) {
  return fmax<>(x, y);
}
abacus_double8 ABACUS_API __abacus_fmax(abacus_double8 x, abacus_double y) {
  return fmax<>(x, y);
}
abacus_double16 ABACUS_API __abacus_fmax(abacus_double16 x, abacus_double y) {
  return fmax<>(x, y);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
