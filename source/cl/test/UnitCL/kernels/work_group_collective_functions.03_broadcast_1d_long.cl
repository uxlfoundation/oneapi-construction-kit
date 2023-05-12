// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
kernel void broadcast_1d_long(global long *in, global size_t *idx, global long *out) {
  const size_t gid = get_global_id(0);
  const size_t wgid = get_group_id(0);
  out[gid] = work_group_broadcast(in[gid], idx[wgid]);
}
