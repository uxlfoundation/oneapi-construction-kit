// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void scalar_select_transform(__global int4 *a, __global int4 *b,
                                      __global int4 *c) {
  int gid = get_global_id(0);
  int4 tmp;
  if (gid % 2 == 0) {
    tmp = a[gid];
  } else {
    tmp = b[gid];
  }

  c[gid] = tmp;
}
