// Copyright (C) Codeplay Software Limited. All Rights Reserved.

int perform_add(int a, int b) { return a + b; }

__kernel void real_adding_kernel(__global int *in1, __global int *in2,
                                 __global int *out) {
  size_t tid = get_global_id(0);
  out[tid] = perform_add(in1[tid], in2[tid]);
}

__attribute__((reqd_work_group_size(16, 1, 1)))
__kernel void unused_kernel(__global int* in, __global int* out) {
  size_t tid = get_global_id(0);
  real_adding_kernel(in, in, out);
}

__attribute__((reqd_work_group_size(8, 1, 1)))
__kernel void multikernel_wgs(__global int *in1, __global int *in2,
                          __global int *out) {
  real_adding_kernel(in1, in2, out);
}
