// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
kernel void reduce_add(global TYPE *in, global TYPE *out) {
  const size_t glid = get_global_linear_id();
  out[glid] = work_group_reduce_add(in[glid]);
}
