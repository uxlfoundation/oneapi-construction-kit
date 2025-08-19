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
//
// CLC OPTIONS: -cl-fast-relaxed-math
// SPIRV OPTIONS: -cl-fast-relaxed-math

// The purpose of this test is to exercise builtin functions that may be
// replaced by LLVM intrinsics, which may in turn be replaced by calls to
// libm.  That can either only happen when libm is accessible (e.g. by the
// loader linking in libm functions).
//
// This test specifically requires fast-math because that gives the compiler
// more lee-way to do the replacement.

// To avoid needing to account for ULP errors in the various functions being
// tested, just compare the result against an impossible value (for the given
// inputs, which the compiler does not know).  For the trig functions a smart
// compiler could maybe figure out that the comparison is impossible, but at
// the time of writing ComputeAorta is not that smart.
#define BIG_VALF 5.0f

kernel void libm_functions(__global float* in, __global float* out) {
  out[0] = exp(in[0]) >= BIG_VALF ? 2.0f : 1.0f;
  out[1] = native_exp(in[1]) >= BIG_VALF ? 2.0f : 1.0f;
  out[2] = exp2(in[2]) >= BIG_VALF ? 2.0f : 1.0f;
  out[3] = native_exp2(in[3]) >= BIG_VALF ? 2.0f : 1.0f;
  out[4] = log(in[4]) >= BIG_VALF ? 2.0f : 1.0f;
  out[5] = native_log(in[5]) >= BIG_VALF ? 2.0f : 1.0f;
  out[6] = log2(in[6]) >= BIG_VALF ? 2.0f : 1.0f;
  out[7] = native_log2(in[7]) >= BIG_VALF ? 2.0f : 1.0f;
  out[8] = log10(in[8]) >= BIG_VALF ? 2.0f : 1.0f;
  out[9] = native_log10(in[9]) >= BIG_VALF ? 2.0f : 1.0f;
  out[10] = sin(in[10]) >= BIG_VALF ? 2.0f : 1.0f;
  out[11] = native_sin(in[11]) >= BIG_VALF ? 2.0f : 1.0f;
  out[12] = cos(in[12]) >= BIG_VALF ? 2.0f : 1.0f;
  out[13] = native_cos(in[13]) >= BIG_VALF ? 2.0f : 1.0f;
}
