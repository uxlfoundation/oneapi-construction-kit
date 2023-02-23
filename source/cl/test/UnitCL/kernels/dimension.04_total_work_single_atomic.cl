// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void total_work_single_atomic(volatile __global unsigned int *count) {
  // UnitCL ensures that count is 0 before the kernel starts (see
  // GenericStreamer<T, V>::PopulateBuffer).
  //
  // See the test body for discussion of the legality of this atomic.
  atomic_inc(count);
}
