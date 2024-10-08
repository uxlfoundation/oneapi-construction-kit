// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct fabs_helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct fabs_helper<T, abacus_half> {
  typedef typename TypeTraits<T>::UnsignedType UIntTy;
  static T _(const T x) {
    const UIntTy xAsUInt = abacus::detail::cast::as<UIntTy>(x);
    const UIntTy abs = xAsUInt & (UIntTy)0x7fff;
    return abacus::detail::cast::as<T>(abs);
  }
};
#endif

template <typename T>
struct fabs_helper<T, abacus_float> {
  typedef typename TypeTraits<T>::UnsignedType UIntTy;
  static T _(const T x) {
    const UIntTy xAsUInt = abacus::detail::cast::as<UIntTy>(x);
    const UIntTy abs = xAsUInt & (UIntTy)0x7fffffff;
    return abacus::detail::cast::as<T>(abs);
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct fabs_helper<T, abacus_double> {
  typedef typename TypeTraits<T>::UnsignedType UIntTy;
  static T _(const T x) {
    const UIntTy xAsUInt = abacus::detail::cast::as<UIntTy>(x);
    const UIntTy abs = xAsUInt & (UIntTy)0x7fffffffffffffff;
    return abacus::detail::cast::as<T>(abs);
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T fabs_(const T x) {
  return fabs_helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_fabs(abacus_half x) { return fabs_(x); }

abacus_half2 ABACUS_API __abacus_fabs(abacus_half2 x) { return fabs_(x); }

abacus_half3 ABACUS_API __abacus_fabs(abacus_half3 x) { return fabs_(x); }

abacus_half4 ABACUS_API __abacus_fabs(abacus_half4 x) { return fabs_(x); }

abacus_half8 ABACUS_API __abacus_fabs(abacus_half8 x) { return fabs_(x); }

abacus_half16 ABACUS_API __abacus_fabs(abacus_half16 x) { return fabs_(x); }
#endif

abacus_float ABACUS_API __abacus_fabs(abacus_float x) { return fabs_(x); }

abacus_float2 ABACUS_API __abacus_fabs(abacus_float2 x) { return fabs_(x); }

abacus_float3 ABACUS_API __abacus_fabs(abacus_float3 x) { return fabs_(x); }

abacus_float4 ABACUS_API __abacus_fabs(abacus_float4 x) { return fabs_(x); }

abacus_float8 ABACUS_API __abacus_fabs(abacus_float8 x) { return fabs_(x); }

abacus_float16 ABACUS_API __abacus_fabs(abacus_float16 x) { return fabs_(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_fabs(abacus_double x) { return fabs_(x); }

abacus_double2 ABACUS_API __abacus_fabs(abacus_double2 x) { return fabs_(x); }

abacus_double3 ABACUS_API __abacus_fabs(abacus_double3 x) { return fabs_(x); }

abacus_double4 ABACUS_API __abacus_fabs(abacus_double4 x) { return fabs_(x); }

abacus_double8 ABACUS_API __abacus_fabs(abacus_double8 x) { return fabs_(x); }

abacus_double16 ABACUS_API __abacus_fabs(abacus_double16 x) { return fabs_(x); }
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
