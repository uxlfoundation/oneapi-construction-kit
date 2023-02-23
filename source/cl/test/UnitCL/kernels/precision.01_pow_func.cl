// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: double

__kernel void pow_func(__global double* x, __global double* y, __global double* out) {
  size_t id = get_global_id(0);
  out[id] = pow(x[id], y[id]);
}
