// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This kernel will never be executed in this test, it is to test the compiler.
__kernel void otherkernel(__global float *out, float val) {
  out[get_global_id(0)] = val;
}

__kernel void twokernel_both_primitive(__global float *out, float val) {
  out[get_global_id(0)] = val;
}
