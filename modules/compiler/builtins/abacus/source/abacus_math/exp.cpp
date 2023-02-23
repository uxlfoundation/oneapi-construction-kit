// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>

#include <abacus/internal/exp_unsafe.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct helper<T, abacus_half> {
  typedef typename TypeTraits<T>::SignedType SignedType;
  static T _(const T x) {
    T result = abacus::internal::exp_unsafe(x);

    // Return INF on overflow, max half value is 65504
    // 0x498b ==> 11.0859, exp(11.0859) ==> 65244.xxx
    //
    // Next value after overflows:
    // 0x498c ==> 11.0938, exp(11.0938) ==> 65762.xxx
    const T hi_limit = abacus::detail::cast::as<T>((SignedType)0x498b);
    const SignedType cond1 = x > hi_limit;
    const T half_infinity = abacus::detail::cast::as<T>((SignedType)0x7C00);
    result = __abacus_select(result, half_infinity, cond1);

    // Return zero on underflow, min half value is 2^-24(~5.96046e-08)
    // 0xcc28 ==> -16.625, exp(-16.625) ==> 6.023573837886479e-08
    //
    // Next value after underflows:
    // 0xcc29 ==> -16.6406, exp(-16.6406) ==> 5.930335237965972e-08
    const T low_limit = abacus::detail::cast::as<T>((SignedType)0xcc28);
    const SignedType cond2 = x < low_limit;
    result = __abacus_select(result, 0.0f16, cond2);

    return result;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct helper<T, abacus_float> {
  static T _(const T x) {
    T result = abacus::internal::exp_unsafe(x);
    result = __abacus_select(result, 0.0f, x < -110.0f);
    return __abacus_select(result, ABACUS_INFINITY, x > 89.0f);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct helper<T, abacus_double> {
  static T _(const T x) {
    typedef typename TypeTraits<T>::SignedType SignedType;

    T result = abacus::internal::exp_unsafe(x);

    const SignedType cond1 = x > 710.0;
    result = __abacus_select(result, (T)ABACUS_INFINITY, cond1);

    const SignedType cond2 = x < -745.0;
    result = __abacus_select(result, (T)0.0, cond2);

    return result;
  }
};
#endif // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
T exp(const T x) {
  return helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_exp(abacus_half x) { return exp<>(x); }
abacus_half2 ABACUS_API __abacus_exp(abacus_half2 x) { return exp<>(x); }
abacus_half3 ABACUS_API __abacus_exp(abacus_half3 x) { return exp<>(x); }
abacus_half4 ABACUS_API __abacus_exp(abacus_half4 x) { return exp<>(x); }
abacus_half8 ABACUS_API __abacus_exp(abacus_half8 x) { return exp<>(x); }
abacus_half16 ABACUS_API __abacus_exp(abacus_half16 x) { return exp<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_exp(abacus_float x) { return exp<>(x); }
abacus_float2 ABACUS_API __abacus_exp(abacus_float2 x) { return exp<>(x); }
abacus_float3 ABACUS_API __abacus_exp(abacus_float3 x) { return exp<>(x); }
abacus_float4 ABACUS_API __abacus_exp(abacus_float4 x) { return exp<>(x); }
abacus_float8 ABACUS_API __abacus_exp(abacus_float8 x) { return exp<>(x); }
abacus_float16 ABACUS_API __abacus_exp(abacus_float16 x) { return exp<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_exp(abacus_double x) { return exp<>(x); }
abacus_double2 ABACUS_API __abacus_exp(abacus_double2 x) { return exp<>(x); }
abacus_double3 ABACUS_API __abacus_exp(abacus_double3 x) { return exp<>(x); }
abacus_double4 ABACUS_API __abacus_exp(abacus_double4 x) { return exp<>(x); }
abacus_double8 ABACUS_API __abacus_exp(abacus_double8 x) { return exp<>(x); }
abacus_double16 ABACUS_API __abacus_exp(abacus_double16 x) { return exp<>(x); }
#endif // __CA_BUILTINS_DOUBLE_SUPPORT
