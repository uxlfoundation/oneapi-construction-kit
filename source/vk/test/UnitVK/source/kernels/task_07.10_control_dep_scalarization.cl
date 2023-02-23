// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void control_dep_scalarization(__global int4 *in, __global int4 *out) {
  size_t tid = get_global_id(0);
  out[tid * 4] = in[tid] * 2;
}
