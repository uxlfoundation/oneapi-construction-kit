// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void scalar_select_transform_2(__global int *a, __global int *b,
                                        __global int *c, int clear) {
  size_t gid = get_global_id(0);
  if (clear != 0) {
    b[gid] = clear;
    c[gid] = clear;
  }

  if (gid >= 125) {
    if (gid % 2 == 0) {
      b[gid] = a[gid];
    } else {
      c[gid] = a[gid];
    }
  }
}
