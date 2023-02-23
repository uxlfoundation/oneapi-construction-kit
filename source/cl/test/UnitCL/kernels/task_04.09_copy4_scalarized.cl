// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy4_scalarized(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);
  size_t base = tid * 4;

  int x = in[base + 0];
  int y = in[base + 1];
  int z = in[base + 2];
  int w = in[base + 3];

  out[base + 0] = x;
  out[base + 1] = y;
  out[base + 2] = z;
  out[base + 3] = w;
}
