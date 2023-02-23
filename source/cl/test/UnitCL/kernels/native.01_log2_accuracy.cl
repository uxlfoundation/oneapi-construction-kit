// Copyright (C) Codeplay Software Limited. All Rights Reserved.

void kernel log2_accuracy(global float *in, global float *out) {
  size_t gid = get_global_id(0);
  out[gid] = native_log2(in[gid]);
}
