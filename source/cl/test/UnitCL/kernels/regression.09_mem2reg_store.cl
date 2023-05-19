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

// SPIR OPTIONS: "-w"
// SPIRV OPTIONS: "-w"

struct S2 {
  short g_34;
  int g_74[7];
  int g_86;
  char16 g_95;
  int g_124[4];
};

__kernel void mem2reg_store(__global ulong *result) {
  struct S2 c_640;
  struct S2 *p_639 = &c_640;
  struct S2 c_641 = {5, {}, (int)p_639, 0, {}};
  p_639->g_34 = 42;
  result[get_global_id(0)] = p_639->g_34;
}
