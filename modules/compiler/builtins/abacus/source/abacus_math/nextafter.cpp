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

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T>
inline T nextafter_helper_scalar(const T x, const T y) {
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef typename TypeTraits<T>::SignedType SignedType;
  static_assert(TypeTraits<T>::num_elements == 1,
                "Function should only be used for scalar types");
  if (__abacus_isnan(y) || __abacus_isnan(x)) {
    return FPShape<T>::NaN();
  }

  const SignedType xInt = abacus::detail::cast::as<SignedType>(x);
  const SignedType yInt = abacus::detail::cast::as<SignedType>(y);

  const UnsignedType xIntAbs =
      abacus::detail::cast::as<UnsignedType>(__abacus_fabs(x));

  // if x == y, or if they're both 0 but with a different sign, return x
  if (__abacus_isftz()) {
    // Target flushes denormals to zero
    const UnsignedType yIntAbs =
        abacus::detail::cast::as<UnsignedType>(__abacus_fabs(y));
    if ((xInt == yInt) || (0 == xIntAbs && 0 == yIntAbs)) {
      return x;
    }
  } else {
    // Target supports denormals
    if (x == y) {
      return x;
    }
  }

  if (0 == xIntAbs) {
    const T one = abacus::detail::cast::as<T>(SignedType(1));
    return __abacus_copysign(one, y);
  }

  // figure out the direction
  SignedType add_one;
  if (__abacus_isftz()) {
    add_one = (xInt < yInt) && ((yInt < 0) || (xInt > 0)) ? 1 : -1;
  } else {
    add_one = (x < 0) ^ (x < y) ? 1 : -1;
  }

  const SignedType result = xInt + add_one;
  return abacus::detail::cast::as<T>(result);
}

template <typename T>
inline T nextafter_helper_vector(const T x, const T y) {
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef typename TypeTraits<T>::SignedType SignedType;
  static_assert(TypeTraits<T>::num_elements != 1,
                "Function should only be used for vector types");
  const SignedType xInt = abacus::detail::cast::as<SignedType>(x);
  const SignedType yInt = abacus::detail::cast::as<SignedType>(y);

  const UnsignedType xIntAbs =
      abacus::detail::cast::as<UnsignedType>(__abacus_fabs(x));
  const UnsignedType yIntAbs =
      abacus::detail::cast::as<UnsignedType>(__abacus_fabs(y));

  SignedType add_one;
  // figure out the direction
  if (__abacus_isftz()) {
    add_one = __abacus_select(SignedType(-1), SignedType(1),
                              (xInt < yInt) & ((yInt < 0) | (xInt > 0)));
  } else {
    add_one = __abacus_select(SignedType(-1), SignedType(1), (x < 0) ^ (x < y));
  }

  // Add one in the right direction to the integer representation and cast back
  // to float.
  T result = abacus::detail::cast::as<T>(xInt + add_one);

  // if x == 0, negative or positive, return 1 with the sign of y
  result = __abacus_select(
      result, __abacus_copysign(abacus::detail::cast::as<T>(SignedType(1)), y),
      xIntAbs == 0);

  // if x == y, or if they're both 0 but with a different sign, return x
  result = __abacus_select(result, x,
                           (xInt == yInt) | ((xIntAbs == 0) & (yIntAbs == 0)));

  // if x or y is NaN, return NaN
  result = __abacus_select(result, FPShape<T>::NaN(),
                           __abacus_isnan(y) | __abacus_isnan(x));
  return result;
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_nextafter(abacus_half x, abacus_half y) {
  return nextafter_helper_scalar(x, y);
}
abacus_half2 ABACUS_API __abacus_nextafter(abacus_half2 x, abacus_half2 y) {
  return nextafter_helper_vector(x, y);
}
abacus_half3 ABACUS_API __abacus_nextafter(abacus_half3 x, abacus_half3 y) {
  return nextafter_helper_vector(x, y);
}
abacus_half4 ABACUS_API __abacus_nextafter(abacus_half4 x, abacus_half4 y) {
  return nextafter_helper_vector(x, y);
}
abacus_half8 ABACUS_API __abacus_nextafter(abacus_half8 x, abacus_half8 y) {
  return nextafter_helper_vector(x, y);
}
abacus_half16 ABACUS_API __abacus_nextafter(abacus_half16 x, abacus_half16 y) {
  return nextafter_helper_vector(x, y);
}
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_nextafter(abacus_float x, abacus_float y) {
  return nextafter_helper_scalar(x, y);
}
abacus_float2 ABACUS_API __abacus_nextafter(abacus_float2 x, abacus_float2 y) {
  return nextafter_helper_vector(x, y);
}
abacus_float3 ABACUS_API __abacus_nextafter(abacus_float3 x, abacus_float3 y) {
  return nextafter_helper_vector(x, y);
}
abacus_float4 ABACUS_API __abacus_nextafter(abacus_float4 x, abacus_float4 y) {
  return nextafter_helper_vector(x, y);
}
abacus_float8 ABACUS_API __abacus_nextafter(abacus_float8 x, abacus_float8 y) {
  return nextafter_helper_vector(x, y);
}
abacus_float16 ABACUS_API __abacus_nextafter(abacus_float16 x,
                                             abacus_float16 y) {
  return nextafter_helper_vector(x, y);
}

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_nextafter(abacus_double x, abacus_double y) {
  return nextafter_helper_scalar(x, y);
}
abacus_double2 ABACUS_API __abacus_nextafter(abacus_double2 x,
                                             abacus_double2 y) {
  return nextafter_helper_vector(x, y);
}
abacus_double3 ABACUS_API __abacus_nextafter(abacus_double3 x,
                                             abacus_double3 y) {
  return nextafter_helper_vector(x, y);
}
abacus_double4 ABACUS_API __abacus_nextafter(abacus_double4 x,
                                             abacus_double4 y) {
  return nextafter_helper_vector(x, y);
}
abacus_double8 ABACUS_API __abacus_nextafter(abacus_double8 x,
                                             abacus_double8 y) {
  return nextafter_helper_vector(x, y);
}
abacus_double16 ABACUS_API __abacus_nextafter(abacus_double16 x,
                                              abacus_double16 y) {
  return nextafter_helper_vector(x, y);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
