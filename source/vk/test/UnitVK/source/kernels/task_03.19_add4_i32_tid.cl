// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void add4_i32_tid(__global int4 *in1, __global int4 *in2,
                           __global int4 *out) {
  int tid = get_global_id(0);

  out[tid] = in1[tid] + in2[tid];
}
