// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void control_dep_packetization(__global int *in, __global int *out,
                                        uint target) {
  size_t tid = get_global_id(0);
  if (tid != target) {
    out[tid] = in[tid] * 2;
  }
}
