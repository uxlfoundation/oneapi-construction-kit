// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void scalar_masked_store_varying(__global int *out, uint target) {
  size_t tid = get_global_id(0);
  if (tid != target) {
    out[tid] = tid + target;
  }
}
