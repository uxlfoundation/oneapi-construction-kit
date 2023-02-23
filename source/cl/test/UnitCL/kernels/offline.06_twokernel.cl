// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This kernel will never be executed in this test, it is to test the compiler.
__kernel void otherkernel(__global float *out) { out[get_global_id(0)] = 6.4f; }

__kernel void twokernel(__global float *out) { out[get_global_id(0)] = 7.4f; }
