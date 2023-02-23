// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void shuffle_constant(__global int4 *in1, __global int2 *in2,
                               __global int2 *out) {
  size_t gid = get_global_id(0);

  int4 splat = shuffle(in2[gid], (uint4)(0, 1, 1, 0));
  // The large index is used to test if only the correct bits are taken into
  // consideration for the mask.
  out[gid] = shuffle2(splat, in1[gid], (uint2)(11098, 6));
}
