// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Set reqd_work_group_size to ensure that the segfault was not special to the
// local-size-unknown compile path (it was not).
__attribute__((reqd_work_group_size(17, 1, 1)))
// `tmp` must be in __local memory for the segfault to occur.
__kernel void
offline_local_memcpy_fixed(__local int *tmp, __global int *out) {
  for (size_t i = 0; i < 17; i++) {
    tmp[i] = i;
  }

  // The barrier doesn't actually have anything to do with the segfault, but is
  // required to make the test semantically correct (or not racy at least).
  barrier(CLK_LOCAL_MEM_FENCE);

  // This must be a loop must have a size of at least 17, most likely because
  // for 32-bit systems LLVM considers sizes of <= 16 to be "small" and emits
  // an inline memcpy.
  if (get_global_id(0) == 0) {
    for (size_t i = 0; i < 17; i++) {
      out[i] = tmp[i];
    }
  }
}
