// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#define TRIPS 256

__kernel void saxpy_static_trip(__global float *x, __global float *y,
                                __global float *out, const float a) {
  size_t tid = get_global_id(0);

  float sum = 0.0f;
  for (int i = 0; i < TRIPS; i++) {
    // SAXPY!
    sum += (a * x[i]) + y[i];
  }

  out[tid] = sum;
}
