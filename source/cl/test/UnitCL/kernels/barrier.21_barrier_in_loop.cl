// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void barrier_in_loop(__global uchar *dst) {
  size_t lid = get_local_id(0);
  size_t gid = get_global_id(0);
  size_t lsize = get_local_size(0);

  for (size_t i = 0; i < lsize; ++i) {
    barrier(CLK_LOCAL_MEM_FENCE);
    if (lid == 0) {
      dst[gid + i] = 'A';
    }
  }
}
