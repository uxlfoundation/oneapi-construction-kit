// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CLC OPTIONS: -cl-std=CL3.0
__kernel void flag_global_clear(volatile __global atomic_flag *flag_buffer) {
  uint gid = get_global_id(0);

  atomic_flag_clear_explicit(flag_buffer + gid, memory_order_relaxed,
                             memory_scope_work_item);
}
