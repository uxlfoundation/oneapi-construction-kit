// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void uniform_alloca(__global int *in, __global int *out) {
  size_t gid = get_global_id(0);
  if (gid == 0) {
    out[0] = ((__global int2 *)in)[0].x;
    out[1] = ((__global int2 *)in)[0].y;
  } else {
    out[gid * 2] = 11;
    out[gid * 2 + 1] = 13;
  }
}
