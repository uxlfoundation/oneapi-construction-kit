// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization10(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

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
        if (n == 3) {
          goto j;
        }
      } else {
        if (ret + id >= n) {
          ret /= n * n + ret;
          goto o;
        }
      }
      if (i++ > 3) {
        ret += n * ret;
        goto n;
      }
      o:
        ret++;
    }
    j:
      if (n < 20) {
        ret += n * 2 + 20;
        goto p;
      } else {
        goto q;
      }
    n:
      ret *= 4;
    q:
      if (i > 5) {
        ret++;
        goto r;
      }
  }

r:
  for (int i = 0; i < n / 4; i++) ret++;
  goto s;

f:
  ret /= n;
  goto p;

p:
  for (int i = 0; i < n * 2; i++) ret++;

s:
  out[id] = ret;
}
