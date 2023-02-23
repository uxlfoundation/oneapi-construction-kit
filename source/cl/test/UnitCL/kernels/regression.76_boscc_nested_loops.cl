// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void boscc_nested_loops(__global int *out, int n) {
  size_t gid = get_global_id(0);
  int ret = 1;

  if (gid < n) {
    for (size_t i = 0; i < gid; ++i) {
      size_t x = n * gid;
      for (size_t j = 0; j < gid; ++j) {
        ret += x * j;
      }
    }
  }

  out[gid] = ret;
}
