// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization9(__global int *out, int n) {
  int id = get_global_id(0);
  int i = 0;

  while (1) {
    int j = 0;
    for (; ; i++) {
      if (j++ > n) break;
    }
    if (i++ + id > n) break;
  }

  out[id] = i;
}
