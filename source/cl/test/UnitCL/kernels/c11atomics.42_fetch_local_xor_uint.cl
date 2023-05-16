// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
__kernel void fetch_local_xor_uint(__global uint *input_buffer,
                              __global uint *output_buffer,
                              volatile __local atomic_uint *local_buffer) {
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);
  uint wgid = get_group_id(0);

  if (lid == 0) {
    atomic_init(local_buffer + lid, input_buffer[gid]);
  }
  work_group_barrier(CLK_LOCAL_MEM_FENCE);
  if (lid != 0) {
    atomic_fetch_xor_explicit(local_buffer, input_buffer[gid],
                              memory_order_relaxed, memory_scope_work_item);
  }
  work_group_barrier(CLK_LOCAL_MEM_FENCE);
  if (lid == 0) {
    output_buffer[wgid] = atomic_load_explicit(
        local_buffer, memory_order_relaxed, memory_scope_work_item);
  }
}
