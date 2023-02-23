// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef __ABACUS_INTERNAL_SQRT_UNSAFE_H__
#define __ABACUS_INTERNAL_SQRT_UNSAFE_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>
#include <abacus/internal/check_surrounding_values.h>
#include <abacus/internal/multiply_exact.h>
#include <abacus/internal/rsqrt_initial_guess.h>

namespace abacus {
namespace internal {
template <typename T>
struct sqrt_unsafe_helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <>
struct sqrt_unsafe_helper<abacus_half> {
  template <typename T>
  static T _(const T &x) {
    typedef typename TypeTraits<T>::LargerType LargerType;
    typedef typename TypeTraits<T>::SignedType SignedType;

    // The following algorithm is more complex than it may appear on the
    // surface.
    // In essence there is a rather famous floating point bithack to get a
    // surprisingly good approximation to 1/sqrt(x), and due to the nature of
    // 1/sqrt(x) there also exists a very computer friendly way of computing
    // extra bits of precision off that initial approximation, via
    // Newton-Rhapson iteration. (a way of computing roots of polynomials)
    // So we use this floating point hack to get a good initial guess, and then
    // do several Newton-Rhapson iterations. Finally because we want sqrt(x)
    // as opposed to 1/sqrt(x) at the end we just multiply by x.
    //
    // Because sqrt(x) is expected to be corectly rounded (aka return the exact
    // answer as closly as possible), for the last iteration and subsequent
    // multiplication by x we do it in 32 bit precision. You can do it in 16 bit
    // but you're just faking 32 bit simulation, which though untested can only
    // be slower

    // See more information on this algorithm at
    // https://en.wikipedia.org/wiki/Fast_inverse_square_root,
    // and a rather excellent derivation/discussion at
    // http://h14s.p5r.org/2012/09/0x5f3759df.html?mwh=1

    T result = rsqrt_initial_guess(x);

    // Newton-Raphson Method times 2
    // Approximate 1/sqrt(x)
    result = 0.5f16 * result * (3.0f16 - result * (result * x));
    result = 0.5f16 * result * (3.0f16 - (result * result) * x);

    // Do one iteration in 32-bit precision as we need 0-ulp for this function:
    LargerType result_f = abacus::detail::cast::convert<LargerType>(result);
    LargerType x_f = abacus::detail::cast::convert<LargerType>(x);

    result_f = LargerType(0.5f) * result_f *
               (LargerType(3.0f) - (result_f * result_f) * x_f);

    // 1/sqrt(x) -> sqrt(x) and convert back
    result = abacus::detail::cast::convert<T>(result_f * x_f);

    result = __abacus_select(
        result, x, abacus::detail::cast::convert<SignedType>(x == 0.0f16));

    // NAN returns:
    result =
        __abacus_select(result, FPShape<T>::NaN(),
                        abacus::detail::cast::convert<SignedType>(x < 0.0f16));
    return result;
  }
};
#endif  //__CA_BUILTINS_HALF_SUPPORT

template <>
struct sqrt_unsafe_helper<abacus_float> {
  template <typename T>
  static T _(const T &x) {
    T result = rsqrt_initial_guess(x);

    // Newton-Raphson Method times 3
    // Approximate 1/sqrt(x)
    result = (T)0.5f * result * ((T)3.0f - result * (result * x));
    result = (T)0.5f * result * ((T)3.0f - result * (result * x));

    // 1/sqrt(x) -> sqrt(x)
    // This is rolled into the final Newton-Raphson iteration so it saves us a
    // multiplication, and improves accuracy.
    T rx = result * x;
    result = (T)0.5f * rx * ((T)3.0f - result * rx);

    result = __abacus_select(result, x, x == 0.0f);

    // This selects NAN when x is negative (and non-zero) or NAN.
    return __abacus_select(ABACUS_NAN, result, x >= 0.0f);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <>
struct sqrt_unsafe_helper<abacus_double> {
  template <typename T>
  static T _(const T &x) {
    T rsqrt_value = rsqrt_initial_guess(x);

    // Newton Rhapson iterations:
    rsqrt_value =
        (T)0.5 * rsqrt_value * ((T)3.0 - rsqrt_value * (rsqrt_value * x));
    rsqrt_value =
        (T)0.5 * rsqrt_value * ((T)3.0 - rsqrt_value * (rsqrt_value * x));
    rsqrt_value =
        (T)0.5 * rsqrt_value * ((T)3.0 - rsqrt_value * (rsqrt_value * x));
    rsqrt_value =
        (T)0.5 * rsqrt_value * ((T)3.0 - rsqrt_value * (rsqrt_value * x));

    // Todo. Maybe just do this exactly?
    // We calculate the square root from the inverse square root by multiplying
    // by `x` so we might as well absorb that into the final Newton Raphson
    // iteration and pull it out as a common subexpression. Not only does this
    // save us a multiplication, it gives us a little more numerical accuracy.
    // Sadly it is still not quite accurate enough for the 0 ULPs we require,
    // but it does get us to within 1 ULP.
    T rx = rsqrt_value * x;
    T sqrt_value = (T)0.5 * rx * ((T)3.0 - rsqrt_value * rx);

    T best_value = check_surrounding_values(x, sqrt_value);

    return best_value;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T sqrt_unsafe(const T &x) {
  typedef typename TypeTraits<T>::ElementType ElementType;
  return sqrt_unsafe_helper<ElementType>::_(x);
}
}  // namespace internal
}  // namespace abacus
#endif  //__ABACUS_INTERNAL_SQRT_UNSAFE_H__
