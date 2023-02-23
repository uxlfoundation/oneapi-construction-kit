// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void scatter_gather(int w, __global uint *in, __global uint *out) {
  uint width64 = (w + 63) & 0xFFFFFFC0;
  int global_id = get_global_id(0);
  uint target_width = width64;

  if (global_id < 4) {
    for (int x = 1; x <= w; x++) {
      out[x + (global_id * target_width)] =
          in[(x - 1) + (global_id * target_width)];
    }
  }
}
