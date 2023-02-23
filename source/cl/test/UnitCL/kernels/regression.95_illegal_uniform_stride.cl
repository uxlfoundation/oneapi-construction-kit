// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void illegal_uniform_stride(__global int* out,
                                     __global int* in) {
  size_t x = get_global_id(0);
  int y = x - 1;
  if (y >= 0 ) {
    out[x] = in[(uint)y];
  } else {
    out[x] = 0;
  }
}
