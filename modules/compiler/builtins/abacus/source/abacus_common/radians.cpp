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

#include <abacus/abacus_common.h>
#include <abacus/abacus_detail_common.h>

#define DEF(TYPE)                              \
  TYPE ABACUS_API __abacus_radians(TYPE x) {   \
    return abacus::detail::common::radians(x); \
  }

#ifdef __CA_BUILTINS_HALF_SUPPORT
DEF(abacus_half);
DEF(abacus_half2);
DEF(abacus_half3);
DEF(abacus_half4);
DEF(abacus_half8);
DEF(abacus_half16);
#endif  //  __CA_BUILTINS_HALF_SUPPORT

DEF(abacus_float);
DEF(abacus_float2);
DEF(abacus_float3);
DEF(abacus_float4);
DEF(abacus_float8);
DEF(abacus_float16);

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
DEF(abacus_double);
DEF(abacus_double2);
DEF(abacus_double3);
DEF(abacus_double4);
DEF(abacus_double8);
DEF(abacus_double16);
#endif  //  __CA_BUILTINS_DOUBLE_SUPPORT
