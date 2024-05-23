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
// CLC OPTIONS: -cl-fast-relaxed-math
// SPIRV OPTIONS: -cl-fast-relaxed-math

// The purpose of this test is to exercise builtin native functions where we use
// half as input. We use the same logic as other testing of builtin functions
// where we give opportunity to be replaced by LLVM intrinsics, which may in
// turn be replaced by calls to libm.  That can either only happen when libm is
// accessible (e.g. by the loader linking in libm functions).
//
// This test specifically requires fast-math because that gives the compiler
// more lee-way to do the replacement.

// To avoid needing to account for ULP errors in the various functions being
// tested, just compare the result against an impossible value (for the given
// inputs, which the compiler does not know).  For the trig functions a smart
// compiler could maybe figure out that the comparison is impossible, but at
// the time of writing ComputeAorta is not that smart.

// REQUIRES: half

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#define BIG_VALF 5.0f

kernel void libm_native_half_input(__global half* in, __global uint* out) {
  uint index = 0;

  out[index] = native_cos(in[index]) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_exp(in[index]) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_exp2(in[index]) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_exp10(in[index]) >= BIG_VALF * 3.0f ? 2 : 1; index++;
  out[index] = native_log(in[index]) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_log2(in[index]) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_log10(in[index]) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_rsqrt(in[index]) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_sin(in[index]) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_tan(in[index]) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_recip(in[index]) >= BIG_VALF ? 2 : 1; index++;    
  out[index] = native_sqrt(in[index]) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_divide(in[index], (half)1000.0) >= BIG_VALF ? 2 : 1; index++;
  out[index] = native_powr(in[index], 1) >= BIG_VALF ? 2 : 1; index++;  
}