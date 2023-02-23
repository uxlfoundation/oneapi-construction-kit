// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: double

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
kernel void double_convert_char_rtp(global double *src, global char *dst) {
  size_t i = get_global_id(0);
  dst[i] = convert_char_rtp(src[i]);
}
