// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization14(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

  while (1) {
    if (n > 0) {
      for (int i = 0; i < n; i++) ret++;
    } else {
      if (id == n) {
        goto k;
      }
    }
    if (i++ >= 2) {
      goto l;
    }
  }

k:
  ret += n;

l:
  out[id] = ret;
}
