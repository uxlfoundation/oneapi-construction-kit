// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CLC OPTIONS: -cl-std=CL3.0
__kernel void flag_global_set_once(volatile __global atomic_flag *flag_buffer,
                                   __global int *output_buffer) {
  uint gid = get_global_id(0);

  output_buffer[gid] = atomic_flag_test_and_set_explicit(
      flag_buffer + gid, memory_order_relaxed, memory_scope_work_item);
}
