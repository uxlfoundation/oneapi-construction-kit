// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization13(__global int *out, int n) {
  size_t tid = get_global_id(0);
  size_t size = get_local_size(0);
  out[tid] = 0;
  if (tid + 1 < size) {
    out[tid] = n;
  } else if (tid + 1 == size) {
    size_t leftovers = 1 + (size & 1);
    switch (leftovers) {
      case 2:
        out[tid] = 2 * n + 1;
        // fall through
      case 1:
        out[tid] += 3 * n - 1;
        break;
    }
    switch (leftovers) {
      case 2:
        out[tid] /= n;
      // fall through
      case 1:
        out[tid]--;
        break;
    }
  }
}
