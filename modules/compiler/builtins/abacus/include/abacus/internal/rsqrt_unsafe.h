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

#ifndef __ABACUS_INTERNAL_RSQRT_UNSAFE_H__
#define __ABACUS_INTERNAL_RSQRT_UNSAFE_H__

#include <abacus/abacus_config.h>
#include <abacus/internal/rsqrt_initial_guess.h>

namespace abacus {
namespace internal {

// Need to keep this original function for compatibility,
// gets called directly by some library functions.
template <typename T>
inline T rsqrt_unsafe(const T &x, unsigned newton_raphson_iterations) {
  T result = rsqrt_initial_guess(x);

  // Newton-Raphson Method N times
  for (unsigned i = 0; i < newton_raphson_iterations; i++) {
    result = (T)0.5f * result * ((T)3.0f - (result * result) * x);
  }

  return result;
}

// For non-half call the original function:
template <typename T>
struct rsqrt_unsafe_helper {
  template <typename T2>
  static T2 _(const T2 &x) {
    // N == 3 for float, N == 6 for double
    // Approximate 1/sqrt(x)
    const unsigned s = sizeof(typename TypeTraits<T2>::ElementType);
    const unsigned t = s - (s / 4);
    return rsqrt_unsafe(x, t);
  }
};

#ifdef __CA_BUILTINS_HALF_SUPPORT
// We have to specialise for half precision as it needs a bit more finesse
template <>
struct rsqrt_unsafe_helper<abacus_half> {
  template <typename T>
  inline static T _(const T &x) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    T result = rsqrt_initial_guess(x);

    // Newton-Raphson Method times 2
    // Approximate rsqrt(x)
    // (The order of adds/multiplies here are extrememly important):

    result = 0.5f16 * result * (3.0f16 - (result * result) * x);
    result = result + (0.5f16 * result * (1.0f16 - ((x * result) * result)));

    // The algorithm so far gets us to within the required 1 ulp for nearly
    // every value, however there is one bit pattern that causes issue,
    // this is dealt with now (ends in 0x547)
    // This was determined by testing every value, so the underlaying reason
    // this value fails is unclear
    // Adding 1 ulp to the result returns the correctly rounded answer.
    // This will probably not work on devices where half add/multiply is not
    // RTN, as this bit-pattern results from the very exactly adds/multiplies
    // in the above Newton-Rhapson

    SignedType needs_fix = (abacus::detail::cast::as<UnsignedType>(x) &
                            UnsignedType(0x7FF)) == UnsignedType(0x547);

    result = __abacus_select(
        result,
        abacus::detail::cast::as<T>(UnsignedType(
            abacus::detail::cast::as<UnsignedType>(result) + UnsignedType(1))),
        needs_fix);

    return result;
  }
};
#endif  //__CA_BUILTINS_HALF_SUPPORT

template <typename T>
inline T rsqrt_unsafe(const T &x) {
  typedef typename TypeTraits<T>::ElementType ElementType;
  return rsqrt_unsafe_helper<ElementType>::_(x);
}
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_RSQRT_UNSAFE_H__
