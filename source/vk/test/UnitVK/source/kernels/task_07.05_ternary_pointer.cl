// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void ternary_pointer(global int *cond, global int *trueVal,
                            global int *falseVal, global int *out) {
  size_t tid = get_global_id(0);
  global int *in = cond[tid] ? trueVal : falseVal;

  out[tid] = in[tid];
}
