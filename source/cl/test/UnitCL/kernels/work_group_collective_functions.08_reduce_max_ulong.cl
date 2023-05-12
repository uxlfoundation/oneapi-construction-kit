// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
kernel void reduce_max_ulong(global ulong *in, global ulong *out) {
  const size_t glid = get_global_linear_id();
  out[glid] = work_group_reduce_max(in[glid]);
}
