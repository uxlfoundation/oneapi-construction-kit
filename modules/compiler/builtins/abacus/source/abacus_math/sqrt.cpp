// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/add_exact.h>
#include <abacus/internal/is_denorm.h>
#include <abacus/internal/multiply_exact.h>
#include <abacus/internal/sqrt_unsafe.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_half> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;
    typedef typename TypeTraits<T>::SignedType SignedType;

    T processedX = x;

    // Un-denormalise any small numbers so it passes through the algorithm
    // correctly
    // Do this by multiply by 2^10 if less than a certain threshold (0x0800),
    // and multiply by 2^-5 at the end
    // This insures intermediate results in sqrt_unsafe are non-denormal
    SignedType xSmall =
        abacus::detail::cast::as<UnsignedType>(x) < UnsignedType(0x0800);

    // TODO might need a bit more on hardware that doesn't support denormals
    processedX = __abacus_select(x, x * 1024.0f16, xSmall);

    /* -------------- For devices without denormalsupport---------------------
    //To multiply a denorm by 2^10 the bithack way:
    //OR in an exponent (In this case 0x2C00), for a denormal x this gives us
x*2^10 + 2^4
    UnsignedType denorm_hack = abacus::detail::cast::as<UnsignedType>(x) |
(UnsignedType) 0x2C00;

    T denorm_scaled = abacus::detail::cast::as<T>(denorm_hack) - (T)0.0625f16;
//2^-4

    processedX = __abacus_select(processedX, denorm_scaled,
abacus::detail::cast::convert<UnsignedType>(abacus::detail::cast::as<UnsignedType>(x)
< 0x0400));
-------------------------------------------------------------------------------*/

    T ans = abacus::internal::sqrt_unsafe(processedX);

    ans = __abacus_select(ans, ans * 0.03125f16, xSmall);

    // This fabs is used to prevent an earlier branch check for x == INFINITY,
    // as it happens the sqrt_unsafe returns -INFINTY in this case.
    return __abacus_fabs(ans);
  }
};

#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    // We pre-condition the input by scaling it up or down, to avoid overflows
    // and underflows/subnormals.
    const SignedType xSmall = x < 1.0f;

    T processedX = x * __abacus_select((T)0.0625f, (T)16777216.0f, xSmall);
    T ans = abacus::internal::sqrt_unsafe(processedX);
    ans = ans * __abacus_select((T)4.0f, (T)0.000244140625f, xSmall);

    // sqrt_unsafe already correctly deals with zeroes, negatives and NANs.
    // Only infinity is left to worry about.
    ans = __abacus_select(ans, x, (x == ABACUS_INFINITY));

    return ans;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    const SignedType xSmall =
        x < __abacus_as_double((abacus_long)0x3CD0000000000000);

    const T inter =
        __abacus_select(x, x * 1267650600228229401496703205376.0, xSmall);

    T result = abacus::internal::sqrt_unsafe(inter);

    result = __abacus_select(
        result, result * __abacus_as_double((abacus_long)0x3CD0000000000000),
        xSmall);

    const SignedType cond1 = (x == 0.0) | (x == ABACUS_INFINITY);
    result = __abacus_select(result, x, cond1);

    const SignedType cond2 = (x < 0.0) | __abacus_isnan(x);
    result = __abacus_select(result, (T)ABACUS_NAN, cond2);

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T sqrt(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_sqrt(abacus_half x) { return sqrt<>(x); }
abacus_half2 ABACUS_API __abacus_sqrt(abacus_half2 x) { return sqrt<>(x); }
abacus_half3 ABACUS_API __abacus_sqrt(abacus_half3 x) { return sqrt<>(x); }
abacus_half4 ABACUS_API __abacus_sqrt(abacus_half4 x) { return sqrt<>(x); }
abacus_half8 ABACUS_API __abacus_sqrt(abacus_half8 x) { return sqrt<>(x); }
abacus_half16 ABACUS_API __abacus_sqrt(abacus_half16 x) { return sqrt<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_sqrt(abacus_float x) { return sqrt<>(x); }
abacus_float2 ABACUS_API __abacus_sqrt(abacus_float2 x) { return sqrt<>(x); }
abacus_float3 ABACUS_API __abacus_sqrt(abacus_float3 x) { return sqrt<>(x); }
abacus_float4 ABACUS_API __abacus_sqrt(abacus_float4 x) { return sqrt<>(x); }
abacus_float8 ABACUS_API __abacus_sqrt(abacus_float8 x) { return sqrt<>(x); }
abacus_float16 ABACUS_API __abacus_sqrt(abacus_float16 x) { return sqrt<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_sqrt(abacus_double x) { return sqrt<>(x); }
abacus_double2 ABACUS_API __abacus_sqrt(abacus_double2 x) { return sqrt<>(x); }
abacus_double3 ABACUS_API __abacus_sqrt(abacus_double3 x) { return sqrt<>(x); }
abacus_double4 ABACUS_API __abacus_sqrt(abacus_double4 x) { return sqrt<>(x); }
abacus_double8 ABACUS_API __abacus_sqrt(abacus_double8 x) { return sqrt<>(x); }
abacus_double16 ABACUS_API __abacus_sqrt(abacus_double16 x) {
  return sqrt<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
