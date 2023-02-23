// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization6(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

  while (1) {
    if (i++ & 1) {
      if (n > 2) {
        goto e;
      }
    } else {
      ret += n + 1;
    }
    if (id == n) break;
  }

  ret += n * 2;
  ret /= n;
  goto early;

e:
  ret += n * 4;
  ret -= n;

early:
  out[id] = ret;
}
