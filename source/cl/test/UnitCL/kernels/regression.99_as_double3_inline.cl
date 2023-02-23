// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: double

__kernel void as_double3_inline(__global long *src, __global double *dst) {
  size_t tid = get_global_id(0);
  double3 tmp = as_double3(vload3(tid, src));
  vstore3(tmp, tid, dst);
}
