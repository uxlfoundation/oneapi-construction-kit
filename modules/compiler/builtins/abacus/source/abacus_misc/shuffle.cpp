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

#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_misc.h>
#include <abacus/abacus_type_traits.h>

namespace hidden {
template <typename T>
struct SizeLaundry {
  static const unsigned num_elements;
};

template <typename T>
const unsigned SizeLaundry<T>::num_elements = TypeTraits<T>::num_elements;

template <typename R, typename T, typename M>
R shuffle(const T x, const M m) {
  typedef typename TypeTraits<M>::ElementType MElementType;
  const M maskedM = m % abacus::detail::cast::convert<MElementType>(
                            SizeLaundry<T>::num_elements);

  typedef typename MakeType<abacus_uint, TypeTraits<M>::num_elements>::type
      UintType;
  const UintType castedMaskedM =
      abacus::detail::cast::convert<UintType>(maskedM);

  R r{};

  for (unsigned int i = 0; i < TypeTraits<M>::num_elements; i++) {
    r[i] = x[castedMaskedM[i]];
  }

  return r;
}
}  // namespace hidden

#define DEF_WITH_BOTH_SIZES(TYPE, IN_SIZE, OUT_SIZE)                 \
  TYPE##OUT_SIZE ABACUS_API __abacus_shuffle(                        \
      TYPE##IN_SIZE x, TypeTraits<TYPE##OUT_SIZE>::UnsignedType m) { \
    return hidden::shuffle<TYPE##OUT_SIZE>(x, m);                    \
  }

#define DEF_WITH_SIZE(TYPE, SIZE)    \
  DEF_WITH_BOTH_SIZES(TYPE, SIZE, 2) \
  DEF_WITH_BOTH_SIZES(TYPE, SIZE, 3) \
  DEF_WITH_BOTH_SIZES(TYPE, SIZE, 4) \
  DEF_WITH_BOTH_SIZES(TYPE, SIZE, 8) \
  DEF_WITH_BOTH_SIZES(TYPE, SIZE, 16)

#define DEF(TYPE)        \
  DEF_WITH_SIZE(TYPE, 2) \
  DEF_WITH_SIZE(TYPE, 3) \
  DEF_WITH_SIZE(TYPE, 4) \
  DEF_WITH_SIZE(TYPE, 8) \
  DEF_WITH_SIZE(TYPE, 16)

DEF(abacus_char);
DEF(abacus_uchar);
DEF(abacus_short);
DEF(abacus_ushort);
DEF(abacus_int);
DEF(abacus_uint);
DEF(abacus_long);
DEF(abacus_ulong);
DEF(abacus_float);
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(abacus_double);
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
