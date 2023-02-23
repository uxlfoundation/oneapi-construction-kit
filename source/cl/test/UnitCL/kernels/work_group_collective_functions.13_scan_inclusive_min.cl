// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
kernel void scan_inclusive_min(global TYPE *in, global TYPE *out) {
  const size_t glid = get_global_linear_id();
  out[glid] = work_group_scan_inclusive_min(in[glid]);
}
