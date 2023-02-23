// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct copysign_helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct copysign_helper<T, abacus_half> {
  static T _(const T x, const T y) {
    const T absX = __abacus_fabs(x);

    typedef typename TypeTraits<T>::SignedType IntTy;
    const IntTy signY = abacus::detail::cast::as<IntTy>(y) & (IntTy)0x8000;
    return __abacus_select(absX, -absX, signY);
  }
};
#endif

template <typename T>
struct copysign_helper<T, abacus_float> {
  static T _(const T x, const T y) {
    const T absX = __abacus_fabs(x);

    typedef typename TypeTraits<T>::SignedType IntTy;
    const IntTy signY = abacus::detail::cast::as<IntTy>(y) & (IntTy)0x80000000;
    return __abacus_select(absX, -absX, signY);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct copysign_helper<T, abacus_double> {
  static T _(const T x, const T y) {
    const T absX = __abacus_fabs(x);

    typedef typename TypeTraits<T>::SignedType IntTy;
    const IntTy signY =
        abacus::detail::cast::as<IntTy>(y) & (IntTy)0x8000000000000000;
    return __abacus_select(absX, -absX, signY);
  }
};
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T copysign_(const T x, const T y) {
  return copysign_helper<T>::_(x, y);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_copysign(abacus_half x, abacus_half y) {
  return copysign_(x, y);
}

abacus_half2 ABACUS_API __abacus_copysign(abacus_half2 x, abacus_half2 y) {
  return copysign_(x, y);
}

abacus_half3 ABACUS_API __abacus_copysign(abacus_half3 x, abacus_half3 y) {
  return copysign_(x, y);
}

abacus_half4 ABACUS_API __abacus_copysign(abacus_half4 x, abacus_half4 y) {
  return copysign_(x, y);
}

abacus_half8 ABACUS_API __abacus_copysign(abacus_half8 x, abacus_half8 y) {
  return copysign_(x, y);
}

abacus_half16 ABACUS_API __abacus_copysign(abacus_half16 x, abacus_half16 y) {
  return copysign_(x, y);
}
#endif

abacus_float ABACUS_API __abacus_copysign(abacus_float x, abacus_float y) {
  return copysign_(x, y);
}

abacus_float2 ABACUS_API __abacus_copysign(abacus_float2 x, abacus_float2 y) {
  return copysign_(x, y);
}

abacus_float3 ABACUS_API __abacus_copysign(abacus_float3 x, abacus_float3 y) {
  return copysign_(x, y);
}

abacus_float4 ABACUS_API __abacus_copysign(abacus_float4 x, abacus_float4 y) {
  return copysign_(x, y);
}

abacus_float8 ABACUS_API __abacus_copysign(abacus_float8 x, abacus_float8 y) {
  return copysign_(x, y);
}

abacus_float16 ABACUS_API __abacus_copysign(abacus_float16 x,
                                            abacus_float16 y) {
  return copysign_(x, y);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_copysign(abacus_double x, abacus_double y) {
  return copysign_(x, y);
}

abacus_double2 ABACUS_API __abacus_copysign(abacus_double2 x,
                                            abacus_double2 y) {
  return copysign_(x, y);
}

abacus_double3 ABACUS_API __abacus_copysign(abacus_double3 x,
                                            abacus_double3 y) {
  return copysign_(x, y);
}

abacus_double4 ABACUS_API __abacus_copysign(abacus_double4 x,
                                            abacus_double4 y) {
  return copysign_(x, y);
}

abacus_double8 ABACUS_API __abacus_copysign(abacus_double8 x,
                                            abacus_double8 y) {
  return copysign_(x, y);
}

abacus_double16 ABACUS_API __abacus_copysign(abacus_double16 x,
                                             abacus_double16 y) {
  return copysign_(x, y);
}
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
