// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void masked_loop_uniform(global int *src, global int *dst, int n) {
  int x = get_global_id(0);
  int size = get_global_size(0);
  if ((x > 1) & (x < 7)) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
      sum += src[i];
    }
    dst[x] = sum;
  }
}
