// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// REQUIRES: double
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
