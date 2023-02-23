// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef __ABACUS_INTERNAL_IS_INTEGER_QUICK_H__
#define __ABACUS_INTERNAL_IS_INTEGER_QUICK_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_type_traits.h>

namespace abacus {
namespace internal {

/// @brief Checks if floating point value represents an integer
///
/// This function only works for floating point values with large exponents,
/// where the mantissa bits don't need to be checked since it's impossible to
/// produce a factional component regardless of significand. This is the case
/// when the exponent is larger than the number of mantissa bits.
///
/// As a result smaller integers like `2.0` won't be detected where
/// `exponent >= 0 && exponent < <Number of bits In Mantissa>`.
///
/// @tparam T Floating point type of @p x, may be scalar or vector.
///
/// @param[in] x Floating point input to check
///
/// @return Zero if @p x is not classified as an integer, non-zero otherwise.
template <typename T>
inline typename TypeTraits<T>::SignedType is_integer_quick(const T& x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef FPShape<T> Shape;

  const SignedType xAsInt = abacus::detail::cast::as<SignedType>(x);

  const SignedType exponentBias = Shape::Bias();
  const SignedType exponentMask = Shape::ExponentMask();
  const SignedType mantissaSize = Shape::Mantissa();

  return (((xAsInt & exponentMask) >> mantissaSize) >=
          (exponentBias + mantissaSize));
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_IS_INTEGER_QUICK_H__
