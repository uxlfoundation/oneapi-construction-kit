// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void wait_event_is_barrier_strided(__local int *tmp,
                                            __global const int *in,
                                            __global int *out) {
  size_t lid = get_local_id(0);
  size_t gid = get_global_id(0);
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);
  event_t event;

  // Write into the local temporary buffer.
  tmp[lid] = in[gid];

  // Barrier so we know tmp is filled.
  barrier(CLK_LOCAL_MEM_FENCE);

  // Now do a scatter into the output buffer.
  event = async_work_group_strided_copy(&out[group * size], tmp, size, 1, 0);
  wait_group_events(1, &event);

  // Increment the output buffer. If the wait_group_events is a nop then this
  // might catch it if async_workgroup_strided_copy didn't run on the first
  // thread to execute the builtin only.
  out[gid] += 1;
}
