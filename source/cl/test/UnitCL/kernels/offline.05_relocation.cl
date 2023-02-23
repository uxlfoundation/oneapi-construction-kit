// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Needs an absolute relocation to a different section
// (between .text and .rodata)
__constant int table[] = {0, 0, 0, 0};

__kernel void relocation(__global int *in1, __global int *in2,
                         __global int *out) {
  size_t tid = get_global_id(0);
  out[tid] = in1[tid] + in2[tid] + table[tid % 4];
}
