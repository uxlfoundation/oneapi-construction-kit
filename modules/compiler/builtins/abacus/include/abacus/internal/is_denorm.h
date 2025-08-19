// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef __ABACUS_INTERNAL_IS_DENORM_H__
#define __ABACUS_INTERNAL_IS_DENORM_H__

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct denorm_helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct denorm_helper<T, abacus_half> {
  typedef typename TypeTraits<T>::SignedType IntTy;
  static IntTy _(const T &x) {
    const IntTy xAsInt = abacus::detail::cast::as<IntTy>(x);
    const IntTy exponentMask = 0x7C00;
    const IntTy mantissaMask = 0x03FF;

    const IntTy exponentZero = ((xAsInt & exponentMask) == 0);
    const IntTy mantissaNonZero = ((xAsInt & mantissaMask) != 0);

    return exponentZero & mantissaNonZero;
  }
};
#endif

template <typename T>
struct denorm_helper<T, abacus_float> {
  typedef typename TypeTraits<T>::SignedType IntTy;
  static IntTy _(const T &x) {
    const IntTy xAsInt = abacus::detail::cast::as<IntTy>(x);

    const IntTy exponentMask = 0x7F800000;
    const IntTy mantissaMask = 0x007FFFFF;

    const IntTy exponentZero = ((xAsInt & exponentMask) == 0);
    const IntTy mantissaNonZero = ((xAsInt & mantissaMask) != 0);

    return exponentZero & mantissaNonZero;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct denorm_helper<T, abacus_double> {
  typedef typename TypeTraits<T>::SignedType IntTy;
  static IntTy _(const T &x) {
    const IntTy xAsInt = abacus::detail::cast::as<IntTy>(x);

    const IntTy exponentMask = 0x7FF0000000000000;
    const IntTy mantissaMask = 0x000FFFFFFFFFFFFF;

    const IntTy exponentZero = ((xAsInt & exponentMask) == 0);
    const IntTy mantissaNonZero = ((xAsInt & mantissaMask) != 0);

    return exponentZero & mantissaNonZero;
  }
};
#endif

template <typename T>
inline typename TypeTraits<T>::SignedType isDenorm(const T &x) {
  return denorm_helper<T>::_(x);
}
}  // namespace

namespace abacus {
namespace internal {
#ifdef __CA_BUILTINS_HALF_SUPPORT
inline abacus_short ABACUS_API is_denorm(abacus_half x) {
  return isDenorm<>(x);
}

inline abacus_short2 ABACUS_API is_denorm(abacus_half2 x) {
  return isDenorm<>(x);
}

inline abacus_short3 ABACUS_API is_denorm(abacus_half3 x) {
  return isDenorm<>(x);
}

inline abacus_short4 ABACUS_API is_denorm(abacus_half4 x) {
  return isDenorm<>(x);
}

inline abacus_short8 ABACUS_API is_denorm(abacus_half8 x) {
  return isDenorm<>(x);
}

inline abacus_short16 ABACUS_API is_denorm(abacus_half16 x) {
  return isDenorm<>(x);
}
#endif

inline abacus_int is_denorm(abacus_float x) { return isDenorm<>(x); }

inline abacus_int2 is_denorm(abacus_float2 x) { return isDenorm<>(x); }

inline abacus_int3 is_denorm(abacus_float3 x) { return isDenorm<>(x); }

inline abacus_int4 is_denorm(abacus_float4 x) { return isDenorm<>(x); }

inline abacus_int8 is_denorm(abacus_float8 x) { return isDenorm<>(x); }

inline abacus_int16 is_denorm(abacus_float16 x) { return isDenorm<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
inline abacus_long is_denorm(abacus_double x) { return isDenorm<>(x); }

inline abacus_long2 is_denorm(abacus_double2 x) { return isDenorm<>(x); }

inline abacus_long3 is_denorm(abacus_double3 x) { return isDenorm<>(x); }

inline abacus_long4 is_denorm(abacus_double4 x) { return isDenorm<>(x); }

inline abacus_long8 is_denorm(abacus_double8 x) { return isDenorm<>(x); }

inline abacus_long16 is_denorm(abacus_double16 x) { return isDenorm<>(x); }
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_IS_DENORM_H__
