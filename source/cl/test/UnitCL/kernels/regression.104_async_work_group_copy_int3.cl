// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This regression test is designed to catch the case that implementations of
// async_work_group_copy respect the padding on vectors with three elements
// mandated by the OpenCL C spec.

kernel void async_work_group_copy_int3(global int3 *in, global int3 *out,
                                       local int3 *tmp) {
  size_t group_id = get_group_id(0);
  size_t size = get_local_size(0);

  // Copy into the local buffer.
  event_t first_event =
      async_work_group_copy(tmp, &in[group_id * size], size, 0);
  wait_group_events(1, &first_event);

  // Then copy straight back out.
  event_t second_event =
      async_work_group_copy(&out[group_id * size], tmp, size, 0);
  wait_group_events(1, &second_event);
}
