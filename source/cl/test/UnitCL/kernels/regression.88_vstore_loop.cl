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

typedef struct {
  uint v[4];
} Foo;

uint4 gen_bits(Foo *ctr)
{
  Foo X;

  for (int i=0;i < 4; i++){
    X.v[i]  = ctr->v[i];
  }

  return (uint4)(X.v[0], X.v[1], X.v[2], X.v[3]);
}

kernel void vstore_loop(global float *output, long out_size)
{
  Foo c = {{1, 1, 1, 1}};

  // output bulk
  unsigned long idx = get_global_id(0)*4;
  while (idx + 4 < out_size)
  {
     float4 ran = convert_float4(gen_bits(&c));
     vstore4(ran, 0, &output[idx]);
     idx += 4*get_global_size(0);
  }

  // output tail
  float4 tail_ran = convert_float4(gen_bits(&c));
  if (idx < out_size)
    output[idx] = tail_ran.x;
  if (idx+1 < out_size)
    output[idx+1] = tail_ran.y;
  if (idx+2 < out_size)
    output[idx+2] = tail_ran.z;
  if (idx+3 < out_size)
    output[idx+3] = tail_ran.w;
}
