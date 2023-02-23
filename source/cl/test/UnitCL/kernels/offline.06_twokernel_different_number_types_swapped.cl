// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This kernel will never be executed in this test, it is to test the compiler.
__kernel void otherkernel(__global float *out) { out[get_global_id(0)] = 6.4f; }

__kernel void twokernel_different_number_types_swapped(__global float *out,
                                                       __global float *out2) {
  out[get_global_id(0)] = 7.4f;
  out2[get_global_id(0)] = 8.4f;
}
