// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void copy(global int *in, global int *out) {
  size_t i = get_global_id(0);
  out[i] = in[i];
}
