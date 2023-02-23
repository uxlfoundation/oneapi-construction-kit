// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void sibling_loops(global int *srcE, global int *srcO, global int *dst) {
  size_t x = get_global_id(0);
  size_t n = get_global_size(0);
  int sum = 0;
  for (size_t i = 0; i <= x; i++) {
    int val;
    if (i & 1)
      val = srcO[i] * 2;
    else
      val = srcE[i] * 3;
    sum += val;
  }
  for (size_t i = x + 1; i < n; i++) {
    int val;
    if (i & 1)
      val = srcE[i] * -5;
    else
      val = srcO[i] * 17;
    sum += val;
  }
  dst[x] = sum;
}
