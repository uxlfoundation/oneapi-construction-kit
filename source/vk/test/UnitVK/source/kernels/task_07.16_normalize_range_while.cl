// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void normalize_range_while(__global int *src, __global int *dst,
                                    int bound) {
  int x = get_global_id(0);
  int val = src[x];
  while (val < 0) {
    val += bound;
  }
  dst[x] = val;
}
