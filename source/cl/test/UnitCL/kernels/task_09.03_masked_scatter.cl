// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void masked_scatter(__global int *a, __global int *b,
                             __global int *b_index) {
  size_t gid = get_global_id(0);
  if (gid % 3 != 0) {
    b[b_index[gid]] = a[gid];
  } else {
    b[b_index[gid]] = 42;
  }
}
