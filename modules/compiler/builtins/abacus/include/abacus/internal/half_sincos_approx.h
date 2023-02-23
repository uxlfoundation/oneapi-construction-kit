// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef __ABACUS_INTERNAL_HALF_SINCOS_APPROX_H__
#define __ABACUS_INTERNAL_HALF_SINCOS_APPROX_H__

#include <abacus/abacus_config.h>

#include <abacus/internal/horner_polynomial.h>

static ABACUS_CONSTANT abacus_float _half_sincos_coefc[4] = {
    1.0f, -0.4999988475f, 0.4165577706e-1f, -0.1359185355e-2f};
static ABACUS_CONSTANT abacus_float _half_sincos_coefs[4] = {
    0.9999999969f, -0.1666665022f, 0.008332016456f, -0.0001950182203f};

namespace abacus {
namespace internal {
template <typename T>
inline T half_sincos_approx(const T& x, T* cosVal) {
  // TODO since sin and cos both call this now it might be worth looking at
  // better ways to make it efficient, as I wrote it quite quickly

  // minimax polynomials from 0 -> PI/4
  const T xx = x * x;
  *cosVal = abacus::internal::horner_polynomial<T, 4>(xx, _half_sincos_coefc);

  return x * abacus::internal::horner_polynomial<T, 4>(xx, _half_sincos_coefs);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_HALF_SINCOS_APPROX_H__
