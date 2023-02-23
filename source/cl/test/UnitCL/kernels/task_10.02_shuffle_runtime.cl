// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void shuffle_runtime(__global int4 *in1, __global int2 *in2,
                              uint4 mask1, uint2 mask2, __global int2 *out) {
  size_t gid = get_global_id(0);

  int4 splat = shuffle(in2[gid], mask1);
  out[gid] = shuffle2(splat, in1[gid], mask2);
}
