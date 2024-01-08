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

#include <abacus/abacus_detail_relational.h>
#include <abacus/abacus_relational.h>

#define DEF(TYPE)                              \
  abacus_int ABACUS_API __abacus_all(TYPE x) { \
    return abacus::detail::relational::all(x); \
  }

DEF(abacus_char);
DEF(abacus_char2);
DEF(abacus_char3);
DEF(abacus_char4);
DEF(abacus_char8);
DEF(abacus_char16);

DEF(abacus_short);
DEF(abacus_short2);
DEF(abacus_short3);
DEF(abacus_short4);
DEF(abacus_short8);
DEF(abacus_short16);

DEF(abacus_int);
DEF(abacus_int2);
DEF(abacus_int3);
DEF(abacus_int4);
DEF(abacus_int8);
DEF(abacus_int16);

DEF(abacus_long);
DEF(abacus_long2);
DEF(abacus_long3);
DEF(abacus_long4);
DEF(abacus_long8);
DEF(abacus_long16);
