// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
kernel void broadcast_2d(global TYPE *in, global uint2 *idx, global TYPE *out) {
  const size_t glid = get_global_linear_id();
  const size_t wgid = get_group_id(0) + get_num_groups(0) * get_group_id(1);
  out[glid] = work_group_broadcast(in[glid], idx[wgid].x, idx[wgid].y);
}
