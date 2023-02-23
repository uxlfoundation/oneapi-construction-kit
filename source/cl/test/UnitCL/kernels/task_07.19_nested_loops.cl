// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void nested_loops(global int *img, global int *stridesX, global int *dst,
                         int width, int height) {
  size_t tid = get_global_id(0);
  size_t strideX = stridesX[tid];
  int sum = 0;
  for (size_t j = 0; j < height; j++) {
    for (size_t i = 0; i < width; i += strideX) {
      sum += img[(j * width) + i];
    }
  }
  dst[tid] = sum;
}
