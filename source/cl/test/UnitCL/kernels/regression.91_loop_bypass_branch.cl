// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Based on task_07.16_normalize_range_while
__kernel void loop_bypass_branch(__global int *src, __global int *dst,
                                    int bound) {
  size_t x = get_global_id(0);
  int val = src[x];
  unsigned v1 = val + 2;

  if (val >= 4) {
    val += 1;
    if (bound == v1) {
      // This branch creates mischief for BOSCC
      goto end;
    }
  }

  while (val < 0) {
    val += bound;
  }

end:
  dst[x] = val;
}
