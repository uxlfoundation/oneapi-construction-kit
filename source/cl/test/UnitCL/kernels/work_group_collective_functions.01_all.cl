// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
kernel void all(global int *in, global int *out) {
  const size_t glid = get_global_linear_id();
  out[glid] = work_group_all(in[glid]);
}
