// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// CLC OPTIONS: -cl-fast-relaxed-math
// SPIR OPTIONS: -cl-fast-relaxed-math
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
