// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void local_array(global int *in, global int *out) {
  size_t tid = get_global_id(0);

  __local int data[1];
  data[0] = in[tid];

  out[tid] = data[0];
}
