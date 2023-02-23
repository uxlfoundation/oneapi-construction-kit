// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef __ABACUS_INTERNAL_ADD_EXACT_H__
#define __ABACUS_INTERNAL_ADD_EXACT_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>

namespace abacus {
namespace internal {
template <typename T>
inline T add_exact(const T x, const T y, T* out_remainder) {
  // Assumes exponent of x >= exponent of y
  const T s = x + y;
  const T z = s - x;

  *out_remainder = y - z;
  return s;
}

// Order of x and y does not matter
template <typename T>
inline void add_exact_safe(T* x, T* y) {
  const T s = *x + *y;
  const T a = s - *y;
  const T b = s - a;
  const T da = *x - a;
  const T db = *y - b;
  const T t = da + db;
  *x = s;
  *y = t;
}

template <typename T>
inline void add_exact(T* x, T* y) {
  T r1_lo{};
  const T r1_hi = add_exact<T>(*x, *y, &r1_lo);
  *x = r1_hi;
  *y = r1_lo;
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_ADD_EXACT_H__
