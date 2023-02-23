// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
kernel void broadcast_1d(global TYPE *in, global size_t *idx, global TYPE *out) {
  const size_t gid = get_global_id(0);
  const size_t wgid = get_group_id(0);
  out[gid] = work_group_broadcast(in[gid], idx[wgid]);
}
