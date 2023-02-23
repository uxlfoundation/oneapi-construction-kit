// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef FLOAT_TYPE
#define FLOAT_TYPE half
#endif

__kernel void half_modf_private(__global FLOAT_TYPE* in,
                                __global FLOAT_TYPE* out,
                                __global FLOAT_TYPE* out_integral) {
  size_t tid = get_global_id(0);
  __private FLOAT_TYPE buffer[1];
  out[tid] = modf(in[tid], &buffer[0]);
  out_integral[tid] = buffer[0];
}
