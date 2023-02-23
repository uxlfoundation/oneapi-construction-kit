// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void mulhi(global int *in1, global int *in2, global int *in3,
                  global int *out) {
  size_t tid = get_global_id(0);

  int a = in1[tid];
  int b = in2[tid];
  int c = in3[tid];
  long temp = (long)a * (long)b;
  int result = (int)(temp >> 32) + c;

  out[tid] = result;
}
