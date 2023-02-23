// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
// SPIRV OPTIONS: -Xclang;-cl-ext=-cl_khr_fp64,+cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void print_halfs(__global half* a, __global half* b) {
  half2 input = (half2)(a[0], b[0]);
  printf("input: (%a, %a)\n", input.x, input.y);
}
