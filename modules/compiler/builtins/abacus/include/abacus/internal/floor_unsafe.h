// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef __ABACUS_INTERNAL_FLOOR_UNSAFE_H__
#define __ABACUS_INTERNAL_FLOOR_UNSAFE_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace abacus {
namespace internal {
template <typename T>
inline typename TypeTraits<T>::SignedType floor_unsafe(const T& x) {
  typedef typename TypeTraits<T>::SignedType SignedType;
  SignedType truncated = abacus::detail::cast::convert<SignedType>(x);
  T diff = __abacus_fabs(x - abacus::detail::cast::convert<T>(truncated));

  SignedType condition = __abacus_isgreater(diff, 0) & __abacus_isless(x, 0);
  SignedType decremented = truncated - 1;
  return __abacus_select(truncated, decremented, condition);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_FLOOR_UNSAFE_H__
