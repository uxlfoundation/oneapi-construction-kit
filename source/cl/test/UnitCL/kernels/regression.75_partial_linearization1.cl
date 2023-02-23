// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization1(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int j = 0;

  while (1) {
    if (id % 2 == 0) {
      if (n > 2) {
        goto e;
      }
    } else {
      for (int i = 0; i < n + 10; i++) ret++;
    }
    if (j++ <= 3) break;
  }

  ret += n * 2;
  for (int i = 0; i < n * 2; i++) ret -= i;
  ret /= n;
  goto early;

e:
  for (int i = 0; i < n + 5; i++) ret /= 2;
  ret -= n;

early:
  out[id] = ret;
}
