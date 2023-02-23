// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

__kernel void onearg_relationals(__global IN_TY* in, __global OUT_TY* out) {
  size_t i = get_global_id(0);
  out[i] = FUNC(in[i]);
}
