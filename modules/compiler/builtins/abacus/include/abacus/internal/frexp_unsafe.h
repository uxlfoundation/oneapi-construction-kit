// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef __ABACUS_INTERNAL_frexp_UNSAFE_H__
#define __ABACUS_INTERNAL_frexp_UNSAFE_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_type_traits.h>

#include <abacus/internal/logb_unsafe.h>

namespace abacus {
namespace internal {

template <typename T, typename N>
inline T frexp_unsafe(const T& x, N* n) {
  using IntVecType =
      typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type;
  using SignedType = typename TypeTraits<T>::SignedType;
  using Shape = FPShape<T>;
  using UnsignedElemType = typename Shape::ScalarUnsignedType;

  IntVecType integerOutputAsI32 = abacus::detail::cast::convert<IntVecType>(
      abacus::internal::logb_unsafe(x) + 1);

  const SignedType xAs = abacus::detail::cast::as<SignedType>(x);

  // A combined mask of the mantissa and sign bits.
  const UnsignedElemType signAndMantissaMask =
      Shape::MantissaMask() | Shape::SignMask();

  const SignedType maskedXAs =
      ((xAs & signAndMantissaMask) | Shape::ZeroPointFive());
  const T result = abacus::detail::cast::as<T>(maskedXAs);

  // Convert the i32 int to the spec defined integer type we are expected to
  // output for type T. For doubles it is i64, for half i16 and for float i32.
  *n = abacus::detail::cast::convert<N>(integerOutputAsI32);
  return result;
}

}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_frexp_UNSAFE_H__
