// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void exchange_local(__global TYPE *in_out_buffer,
                             __global TYPE *desired_buffer,
                             __global TYPE *output_buffer,
                             volatile __local ATOMIC_TYPE *local_buffer) {
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);

  atomic_init(local_buffer + lid, in_out_buffer[gid]);
  output_buffer[gid] =
      atomic_exchange_explicit(local_buffer + lid, desired_buffer[gid],
                               memory_order_relaxed, memory_scope_work_item);
  in_out_buffer[gid] = atomic_load_explicit(
      local_buffer + lid, memory_order_relaxed, memory_scope_work_item);
}
