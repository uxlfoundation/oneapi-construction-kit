// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// SPIR, SPIR-V, and Offline SPIR tests are disabled as this is testing a build
// option only supported by the runtime compiler or clc.

// REQUIRES: nospir; nospirv;
// CLC OPTIONS: -cl-wfv=always

int perform_add(int a, int b) { return a + b; }

kernel void real_adding_kernel(global int *in1, global int *in2,
                               global int *out) {
  size_t tid = get_global_id(0);
  out[tid] = perform_add(in1[tid], in2[tid]);
}

__attribute__((reqd_work_group_size(16, 1, 1)))
kernel void unused_kernel(global int* in, global int* out) {
  size_t tid = get_global_id(0);
  real_adding_kernel(in, in, out);
}

__attribute__((reqd_work_group_size(8, 1, 1)))
kernel void multikernel_wgs_vecz(global int *in1, global int *in2,
                                 global int *out) {
  real_adding_kernel(in1, in2, out);
}
