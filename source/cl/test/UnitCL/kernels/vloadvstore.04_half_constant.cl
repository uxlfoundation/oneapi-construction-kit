// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half parameters
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_constant(__constant half *input,
                            __global half *output) {
  size_t tid = get_global_id(0);
  HALFN tmp = LOADN(tid, input);
  STOREN(tmp, tid, output);
};
