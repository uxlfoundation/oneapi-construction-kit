// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void init_global(__global TYPE *input_buffer,
                          volatile __global ATOMIC_TYPE *output_buffer) {
  uint gid = get_global_id(0);
  atomic_init(output_buffer + gid, input_buffer[gid]);
}
