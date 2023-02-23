// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void normalize_range(__global int *src, __global int *dst, int bound) {
  int x = get_global_id(0);
  int val = src[x];
  do {
    val += bound;
  } while (val < 0);
  dst[x] = val;
}
