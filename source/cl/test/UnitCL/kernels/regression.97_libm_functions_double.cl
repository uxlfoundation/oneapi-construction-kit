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
//
// REQUIRES: double
// CLC OPTIONS: -cl-fast-relaxed-math
// SPIRV OPTIONS: -cl-fast-relaxed-math

// The purpose of this test is to exercise builtin functions that may be
// replaced by LLVM intrinsics, which may in turn be replaced by calls to
// libm.  That can either only happen when libm is accessible (e.g. by the
// loader linking in libm functions).
//
// This test specifically requires fast-math because that gives the compiler
// more lee-way to do the replacement.

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

// To avoid needing to account for ULP errors in the various functions being
// tested, just compare the result against an impossible value (for the given
// inputs, which the compiler does not know).  For the trig functions a smart
// compiler could maybe figure out that the comparison is impossible, but at
// the time of writing ComputeAorta is not that smart.
#define BIG_VAL 5.0

kernel void libm_functions_double(__global double* in, __global double* out) {
  out[0] = exp(in[0]) >= BIG_VAL ? 2.0 : 1.0;
  out[1] = exp2(in[1]) >= BIG_VAL ? 2.0 : 1.0;
  out[2] = log(in[2]) >= BIG_VAL ? 2.0 : 1.0;
  out[3] = log2(in[3]) >= BIG_VAL ? 2.0 : 1.0;
  out[4] = log10(in[4]) >= BIG_VAL ? 2.0 : 1.0;
  out[5] = sin(in[5]) >= BIG_VAL ? 2.0 : 1.0;
  out[6] = cos(in[6]) >= BIG_VAL ? 2.0 : 1.0;
}
