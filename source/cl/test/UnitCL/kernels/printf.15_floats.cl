// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// If a device supports doubles, then printf will implicitly convert floats
// to doubles. If the device does not support doubles then this implicit
// conversion will not happen. However, as we generate and commit SPIR-V on
// a system that does support doubles, that implicit conversion is
// included in the SPIR-V bytecode and is used as part of those tests, even if
// the device doesn't support FP64 (such as Windows).
//
// To work around this issue, we disable the fp64 extension to ensure that the
// generated SPIR-V does not contain this implicit conversion.

// DEFINITIONS: -DNUM_INPUTS=32
// SPIRV OPTIONS: -Xclang;-cl-ext=-cl_khr_fp64;-Wno-format

#ifndef NUM_INPUTS
#define NUM_INPUTS 1
#endif

kernel void floats(global float* in, global float* out) {
  size_t gid = get_global_id(0);
  out[gid] = in[gid] * in[gid];
  int i;
  // We are only printing from one workitem to make sure the order remains
  // constant
  if (gid == 0) {
    for (i = 0; i < NUM_INPUTS; ++i) {
      printf("%#16.1A\n", in[i] * in[i]);
    }
  }
}
