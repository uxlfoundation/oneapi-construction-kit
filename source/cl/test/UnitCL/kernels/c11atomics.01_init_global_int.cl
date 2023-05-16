// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
__kernel void init_global_int(__global int *input_buffer,
                          volatile __global atomic_int *output_buffer) {
  uint gid = get_global_id(0);
  atomic_init(output_buffer + gid, input_buffer[gid]);
}
