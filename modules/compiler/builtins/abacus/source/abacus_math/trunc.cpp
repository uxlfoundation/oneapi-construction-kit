// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>

#include <abacus/internal/is_denorm.h>
#include <abacus/internal/is_integer_quick.h>

namespace {
template <typename T>
inline T trunc_helper_scalar(const T x) {
  typedef typename TypeTraits<T>::SignedType IntTy;
  static_assert(TypeTraits<T>::num_elements == 1,
                "This function should only be called on scalar types");

  if (abacus::internal::is_denorm(x)) {
    return (T)0;
  }

  // Not all floating point numbers representing integers can be stored in a
  // signed integer type of the same size, so converting to `IntTy` and back to
  // the float type `T` will fail. We can avoid this and just return the input.
  if (!__abacus_isnormal(x) || abacus::internal::is_integer_quick(x)) {
    return x;
  }
  return (T)((IntTy)(x));
}

template <typename T>
inline T trunc_helper(const T x) {
  typedef typename TypeTraits<T>::SignedType IntTy;
  static_assert(TypeTraits<T>::num_elements != 1,
                "This function should only be called on vector types");

  const IntTy convInt = abacus::detail::cast::convert<IntTy>(x);
  const T convFloat = abacus::detail::cast::convert<T>(convInt);

  const IntTy identityCond =
      ~__abacus_isnormal(x) | abacus::internal::is_integer_quick(x);
  T r = __abacus_select(convFloat, x, identityCond);

  return __abacus_select(r, (T)0, abacus::internal::is_denorm(x));
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_trunc(abacus_half x) {
  return trunc_helper_scalar(x);
}
abacus_half2 ABACUS_API __abacus_trunc(abacus_half2 x) {
  return trunc_helper(x);
}
abacus_half3 ABACUS_API __abacus_trunc(abacus_half3 x) {
  return trunc_helper(x);
}
abacus_half4 ABACUS_API __abacus_trunc(abacus_half4 x) {
  return trunc_helper(x);
}
abacus_half8 ABACUS_API __abacus_trunc(abacus_half8 x) {
  return trunc_helper(x);
}
abacus_half16 ABACUS_API __abacus_trunc(abacus_half16 x) {
  return trunc_helper(x);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_trunc(abacus_float x) {
  return trunc_helper_scalar(x);
}
abacus_float2 ABACUS_API __abacus_trunc(abacus_float2 x) {
  return trunc_helper(x);
}
abacus_float3 ABACUS_API __abacus_trunc(abacus_float3 x) {
  return trunc_helper(x);
}
abacus_float4 ABACUS_API __abacus_trunc(abacus_float4 x) {
  return trunc_helper(x);
}
abacus_float8 ABACUS_API __abacus_trunc(abacus_float8 x) {
  return trunc_helper(x);
}
abacus_float16 ABACUS_API __abacus_trunc(abacus_float16 x) {
  return trunc_helper(x);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_trunc(abacus_double x) {
  return trunc_helper_scalar(x);
}
abacus_double2 ABACUS_API __abacus_trunc(abacus_double2 x) {
  return trunc_helper(x);
}
abacus_double3 ABACUS_API __abacus_trunc(abacus_double3 x) {
  return trunc_helper(x);
}
abacus_double4 ABACUS_API __abacus_trunc(abacus_double4 x) {
  return trunc_helper(x);
}
abacus_double8 ABACUS_API __abacus_trunc(abacus_double8 x) {
  return trunc_helper(x);
}
abacus_double16 ABACUS_API __abacus_trunc(abacus_double16 x) {
  return trunc_helper(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
