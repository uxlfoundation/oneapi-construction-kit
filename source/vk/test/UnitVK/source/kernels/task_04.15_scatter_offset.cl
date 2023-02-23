// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void scatter_offset(global int *in, global int *out,
                           global uint *offsets) {
  size_t tid = get_global_id(0);
  size_t offset = (size_t)offsets[tid];
  out[offset] = in[tid];
}
