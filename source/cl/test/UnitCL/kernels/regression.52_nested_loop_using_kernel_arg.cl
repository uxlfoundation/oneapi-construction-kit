// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void nested_loop_using_kernel_arg(__global int *src, __global int *dst) {
  int result = 0;
  for (int i = 0; i < 1; i++) {
    for (int j = 0; j < 1; j++) {
      result = *src;
    }
    src++;
  }

  barrier(CLK_LOCAL_MEM_FENCE);
  dst[get_global_id(0)] = result;
}
