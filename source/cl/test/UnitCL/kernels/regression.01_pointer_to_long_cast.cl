// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DN=256"

__kernel void pointer_to_long_cast(__global int *in) {
  __local long ptr_array[N];
  uint tid = get_global_id(0);
  ptr_array[tid] = (long)in;
}
