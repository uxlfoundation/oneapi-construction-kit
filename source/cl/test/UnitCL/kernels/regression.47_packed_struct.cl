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

// SPIRV OPTIONS: "-w"

typedef struct {
  short s;
  float f;
} paddedStruct;

typedef struct {
  char2 c;
  paddedStruct p;
} __attribute__((packed)) packedStruct;

__kernel void packed_struct(__global ulong *out) {
  const size_t gid = get_global_id(0);
  packedStruct s = {('a', 'b'), {42, 3.14f}};
  s.p.s = gid;

  out[gid] = (ulong)(&s.p) - (ulong)(&s);
}
