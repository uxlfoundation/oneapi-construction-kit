// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// SPIR OPTIONS: "-w"
// SPIRV OPTIONS: "-w"

__kernel void bit_shift(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);

  out[tid] = in[tid] << 35;
}
