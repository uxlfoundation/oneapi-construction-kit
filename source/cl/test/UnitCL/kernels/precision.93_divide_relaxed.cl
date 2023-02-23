// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// CLC OPTIONS: -cl-fast-relaxed-math

kernel void divide_relaxed(__global float2* x,
                           __global float2* y,
                           __global float2* out) {
  size_t id = get_global_id(0);
  out[id] = x[id]/y[id];
}
