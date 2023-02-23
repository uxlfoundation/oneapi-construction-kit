// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void other_kernel(__global uint *out, __global const uint *in) {
  size_t gid = get_global_id(0);
  out[gid] = in[gid];
}

__kernel void kernel_call_kernel(__global uint *out, __global const uint *in) {
  other_kernel(out, in);
}
