// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void async_copy(const __global uint *in, __local uint* buffer,
                       __global uint *out) {
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);

  event_t e = async_work_group_strided_copy(buffer, &in[group * size], size, 1, 0);
  wait_group_events(1, &e);
  e = async_work_group_strided_copy(&out[group * size], (const __local uint*)buffer, size, 1, 0);
  wait_group_events(1, &e);
}
