// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void printf_add(__global int *in1, __global int *in2,
                         __global int *out, __global int *status) {
  size_t tid = get_global_id(0);
  int sum = in1[tid] + in2[tid];
  out[tid] = sum;
  status[tid] = printf("sum[%d] = %d\n", (int)tid, sum);
}
