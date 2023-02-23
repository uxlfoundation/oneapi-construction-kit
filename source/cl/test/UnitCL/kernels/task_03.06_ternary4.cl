// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void ternary4(__global int4 *in1, int4 trueVal, int4 falseVal,
                       __global int4 *out) {
  size_t tid = get_global_id(0);

  out[tid] = in1[tid] ? trueVal : falseVal;
}
