// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization16(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

  if (n < 5) {
    for (int i = 0; i < n + 10; i++) ret++;
    goto h;
  } else {
    while (1) {
      if (id + i % 2 == 0) {
        if (n > 2) {
          goto f;
        }
      } else {
        for (int i = 0; i < n + 10; i++) ret++;
      }
      if (n > 5) break;
    }
  }

  ret += n * 2;
  for (int i = 0; i < n * 2; i++) ret -= i;
  ret /= n;
  goto early;

f:
  for (int i = 0; i < n + 5; i++) ret /= 2;
  ret -= n;

h:
  for (int i = 0; i < n * 2; i++) ret -= i;

early:
  out[id] = ret;
}
