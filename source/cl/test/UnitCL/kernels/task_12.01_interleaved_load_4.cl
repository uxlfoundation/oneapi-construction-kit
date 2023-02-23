// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void interleaved_load_4(__global int *out, __global int *in, int stride) {
  int x = get_global_id(0);
  int y = get_global_id(1);

  int v1 = in[(x + y*stride) * 2 + 1];
  int v2 = in[(x + y*stride) * 2];

  out[x + y*stride] = v1 - v2;
}
