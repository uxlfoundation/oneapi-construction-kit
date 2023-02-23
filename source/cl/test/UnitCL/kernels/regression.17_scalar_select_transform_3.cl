// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void scalar_select_transform_3(__global int4 *a, __global int4 *b,
                                        __global int4 *c, int4 clear) {
  size_t gid = get_global_id(0);
  if (clear.x != 0) {
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
