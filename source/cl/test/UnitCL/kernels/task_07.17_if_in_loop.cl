// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void if_in_loop(global int *srcE, global int *srcO, global int *dst) {
  size_t x = get_global_id(0);
  int sum = 0;
  for (size_t i = 0; i <= x; i++) {
    int val;
    if (i & 1)
      val = srcO[x] * 2;
    else
      val = srcE[x] * 3;
    sum += val;
  }
  dst[x] = sum;
}
