// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void right_shift(__global long* out, __global long* lhs,
                          __global long* rhs) {
  const size_t gid = get_global_id(0);
  out[gid] = lhs[gid] >> rhs[gid];
}
