// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef __ABACUS_INTERNAL_TRUNC_UNSAFE_H__
#define __ABACUS_INTERNAL_TRUNC_UNSAFE_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_type_traits.h>

namespace abacus {
namespace internal {
template <typename T>
inline typename TypeTraits<T>::SignedType trunc_unsafe(const T& x) {
  return abacus::detail::cast::convert<typename TypeTraits<T>::SignedType>(x);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_TRUNC_UNSAFE_H__
