// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: double

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void fract_double(__global double* in, __global double* out,
                            __global double* out2) {
  size_t i = get_global_id(0);
  double f0 = in[i];
  double iout = NAN;
  f0 = fract(f0, &iout);
  out[i] = f0;
  out2[i] = iout;
}
