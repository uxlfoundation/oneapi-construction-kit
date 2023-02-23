// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void mad_sat_long(__constant long *a, __constant long *b,
                           __constant long *c, __global long *result) {
  size_t tid = get_global_id(0);
  result[tid] = mad_sat(a[tid], b[tid], c[tid]);
}
