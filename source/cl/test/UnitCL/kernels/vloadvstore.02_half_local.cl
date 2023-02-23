// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half parameters
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_local(__global half *input,
                         __local half *scratch,
                         __global half *output) {
  size_t gid = get_global_id(0);
  HALFN global_copy = LOADN(gid, input);  // vload from __global

  size_t lid = get_local_id(0);
  STOREN(global_copy, lid, scratch);  // vstore to __local

  barrier(CLK_LOCAL_MEM_FENCE);

  HALFN local_copy = LOADN(lid, scratch);  // vload from __local
  STOREN(local_copy, gid, output);  // vstore to __global
};
