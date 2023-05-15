// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
kernel void broadcast_2d_ulong(global ulong *in, global uint2 *idx, global ulong *out) {
  const size_t glid = get_global_linear_id();
  const size_t wgid = get_group_id(0) + get_num_groups(0) * get_group_id(1);
  out[glid] = work_group_broadcast(in[glid], idx[wgid].x, idx[wgid].y);
}
