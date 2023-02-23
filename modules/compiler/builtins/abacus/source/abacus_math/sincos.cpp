// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>

#include <abacus/internal/payne_hanek.h>
#include <abacus/internal/sincos_approx.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct sincos_helper {
  static T _(const T x, T *out_cos) {
    using SignedType = typename TypeTraits<T>::SignedType;

    typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type octet = 0;

    // Range reduction from 0 -> pi/4:
    const T xReduced = abacus::internal::payne_hanek(x, &octet);

    // we use value of both cos_approx and sin_approx in all cases:
    T cosApprox;
    const T sinApprox = abacus::internal::sincos_approx(xReduced, &cosApprox);

    // Depending on octet we have to return different things
    const SignedType cond1 =
        abacus::detail::cast::convert<SignedType>(((octet + 1) & 0x2) == 0);
    const T cosResult = __abacus_select(sinApprox, cosApprox, cond1);
    const T sinResult = __abacus_select(cosApprox, sinApprox, cond1);

    const SignedType cond2 =
        abacus::detail::cast::convert<SignedType>(((octet + 2) & 0x4) == 0);
    *out_cos = __abacus_select(-cosResult, cosResult, cond2);

    const SignedType cond3 =
        abacus::detail::cast::convert<SignedType>((octet & 0x4) == 0);
    return __abacus_select(-sinResult, sinResult, cond3);
  }
};

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct sincos_helper<T, abacus_half> {
  static T _(const T x, T *out_cos) {
    using SignedType = typename TypeTraits<T>::SignedType;

    SignedType octet = 0;

    // Range reduction from 0 -> pi/4:
    const T xReduced = abacus::internal::payne_hanek_half(x, &octet);

    // we use value of both cos_approx and sin_approx in all cases:
    T cosApprox;
    const T sinApprox = abacus::internal::sincos_approx(xReduced, &cosApprox);

    // Depending on octet we have to return different things
    const SignedType cond1 =
        abacus::detail::cast::convert<SignedType>(((octet + 1) & 0x2) == 0);
    const T cosResult = __abacus_select(sinApprox, cosApprox, cond1);
    T sinResult = __abacus_select(cosApprox, sinApprox, cond1);

    // Condition for switching cos sign
    const SignedType cond2 =
        abacus::detail::cast::convert<SignedType>(((octet + 2) & 0x4) == 0);
    *out_cos = __abacus_select(-cosResult, cosResult, cond2);

    // Condition for switching sin sign
    const SignedType cond3((x < 0.0f16) ^ ((octet & 0x7) >= 4));
    sinResult = __abacus_select(sinResult, -sinResult, cond3);

    // When denormals are unavailable, we need to handle the smallest FP16 value
    // explicitly, as otherwise there is a flush to zero.
    sinResult =
        __abacus_select(sinResult, x,
                        SignedType(SignedType(__abacus_isftz()) &&
                                   __abacus_fabs(x) == 6.103515625e-05));

    return sinResult;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
inline T sincos(const T x, T *out_cos) {
  return sincos_helper<T>::_(x, out_cos);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_sincos(abacus_half x, abacus_half* o) {
  return sincos<>(x, o);
}
abacus_half2 ABACUS_API __abacus_sincos(abacus_half2 x, abacus_half2* o) {
  return sincos<>(x, o);
}
abacus_half3 ABACUS_API __abacus_sincos(abacus_half3 x, abacus_half3* o) {
  return sincos<>(x, o);
}
abacus_half4 ABACUS_API __abacus_sincos(abacus_half4 x, abacus_half4* o) {
  return sincos<>(x, o);
}
abacus_half8 ABACUS_API __abacus_sincos(abacus_half8 x, abacus_half8* o) {
  return sincos<>(x, o);
}
abacus_half16 ABACUS_API __abacus_sincos(abacus_half16 x, abacus_half16* o) {
  return sincos<>(x, o);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_sincos(abacus_float x, abacus_float* o) {
  return sincos<>(x, o);
}
abacus_float2 ABACUS_API __abacus_sincos(abacus_float2 x, abacus_float2* o) {
  return sincos<>(x, o);
}
abacus_float3 ABACUS_API __abacus_sincos(abacus_float3 x, abacus_float3* o) {
  return sincos<>(x, o);
}
abacus_float4 ABACUS_API __abacus_sincos(abacus_float4 x, abacus_float4* o) {
  return sincos<>(x, o);
}
abacus_float8 ABACUS_API __abacus_sincos(abacus_float8 x, abacus_float8* o) {
  return sincos<>(x, o);
}
abacus_float16 ABACUS_API __abacus_sincos(abacus_float16 x, abacus_float16* o) {
  return sincos<>(x, o);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_sincos(abacus_double x, abacus_double* o) {
  return sincos<>(x, o);
}
abacus_double2 ABACUS_API __abacus_sincos(abacus_double2 x, abacus_double2* o) {
  return sincos<>(x, o);
}
abacus_double3 ABACUS_API __abacus_sincos(abacus_double3 x, abacus_double3* o) {
  return sincos<>(x, o);
}
abacus_double4 ABACUS_API __abacus_sincos(abacus_double4 x, abacus_double4* o) {
  return sincos<>(x, o);
}
abacus_double8 ABACUS_API __abacus_sincos(abacus_double8 x, abacus_double8* o) {
  return sincos<>(x, o);
}
abacus_double16 ABACUS_API __abacus_sincos(abacus_double16 x,
                                           abacus_double16* o) {
  return sincos<>(x, o);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
