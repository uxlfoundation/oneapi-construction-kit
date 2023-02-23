// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void barrier_in_reduce(__global const int *src, __global int *dst,
                                __local int *block) {
  size_t lid = get_local_id(0);
  size_t gid = get_global_id(0);
  size_t offset = get_local_size(0) >> 1;

  block[lid] = src[gid];

  do {
    barrier(CLK_LOCAL_MEM_FENCE);

    if (lid < offset) {
      int accum = block[lid];
      accum += block[lid + offset];
      block[lid] = accum;
    }

    offset >>= 1;
  } while (offset > 0);

  // Note, no barrier here because only thread 0 wrote to address block[0], and
  // only thread 0 will read block[0] below.  This lack of barrier is
  // essentially the point of the test.

  if (lid == 0) {
    dst[get_group_id(0)] = block[0];
  }
}
