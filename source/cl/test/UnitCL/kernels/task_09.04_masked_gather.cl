// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void masked_gather(__global int *a, __global int *a_index,
                            __global int *b) {
  size_t gid = get_global_id(0);
  if (gid % 3 != 0) {
    b[gid] = a[a_index[gid]];
  } else {
    b[gid] = 42;
  }
}
