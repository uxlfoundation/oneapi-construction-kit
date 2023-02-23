// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization15(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;

  while (1) {
    if (n > 0) {
      for (int i = 0; i < n * 2; i++) ret++;
      if (n <= 10) {
      goto f;
      }
    } else {
      for (int i = 0; i < n / 4; i++) ret++;
    }
    ret++;
    while (1) {
      if (n & 1) {
        if (n < 3) {
          goto l;
        }
      } else {
        if (ret + id >= n) {
          ret /= n * n + ret;
          goto m;
        }
      }
      if (n & 1) {
        goto l;
      }
      m:
        ret++;
    }
    l:
      ret *= 4;
    o:
      if (n & 1) {
        ret++;
        goto p;
      }
  }

p:
  for (int i = 0; i < n / 4; i++) ret++;
  goto q;

f:
  ret /= n;
  goto n;

n:
  for (int i = 0; i < n * 2; i++) ret++;

q:
  out[id] = ret;
}
