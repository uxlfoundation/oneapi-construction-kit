// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void pow_powr(__global float* x,
                     __global float* y,
                     __global float* pow_out,
                     __global float* powr_out) {
  size_t id = get_global_id(0);
  pow_out[id] = pow(x[id], y[id]);
  powr_out[id] = powr(x[id], y[id]);
}
