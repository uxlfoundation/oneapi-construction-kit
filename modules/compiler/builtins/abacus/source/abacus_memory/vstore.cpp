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
#include <abacus/abacus_memory.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T, typename U>
void vstore(const T data, const size_t offset, U p) {
  const U pPlusOffset = p + (offset * TypeTraits<T>::num_elements);

  for (unsigned int i = 0; i < TypeTraits<T>::num_elements; i++) {
    pPlusOffset[i] = data[i];
  }
}
}  // namespace

#define DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, ADDRESS_SPACE) \
  void ABACUS_API __abacus_vstore##SIZE(TYPE##SIZE data, size_t offset,     \
                                        TYPE ADDRESS_SPACE *x) {            \
    return vstore<>(data, offset, x);                                       \
  }

#ifdef __OPENCL_VERSION__
#define DEF_WITH_TYPE_AND_SIZE(TYPE, SIZE)                       \
  DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, __global) \
  DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, __local)  \
  DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, __private)
#else
#define DEF_WITH_TYPE_AND_SIZE(TYPE, SIZE) \
  DEF_WITH_TYPE_AND_SIZE_AND_ADDRESS_SPACE(TYPE, SIZE, )
#endif

#define DEF(TYPE)                 \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 2) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 3) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 4) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 8) \
  DEF_WITH_TYPE_AND_SIZE(TYPE, 16)

DEF(abacus_char)
DEF(abacus_uchar)
DEF(abacus_short)
DEF(abacus_ushort)
DEF(abacus_int)
DEF(abacus_uint)
DEF(abacus_long)
DEF(abacus_ulong)
#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF(abacus_half)
#endif  // __CA_BUILTINS_HALF_SUPPORT
DEF(abacus_float)
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(abacus_double)
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
