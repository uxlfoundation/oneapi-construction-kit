// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void local_vec_mem(__global float *out, __local float4 *buffer,
                            __global float *in) {
  buffer[0].x = *in;
  barrier(CLK_LOCAL_MEM_FENCE);
  *out = buffer[0].x;
}
