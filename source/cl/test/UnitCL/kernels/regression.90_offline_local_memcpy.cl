// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// `tmp` must be in __local memory, it can be either passed as a parameter or
// be statically sized inside the kernel, but it must be __local not __global
// for the segfault to occur.
__kernel void offline_local_memcpy(__local int *tmp, __global int *out) {
  size_t gid = get_global_id(0);
  size_t lid = get_local_id(0);
  size_t size = get_local_size(0);

  // We must save `gid` to `tmp[lid]`.  Simply using `gid` later on is not
  // enough to cause the segfault.  As mentioned above it is important that
  // `tmp` is in __local memory.
  tmp[lid] = (int)gid;

  // The barrier doesn't actually have anything to do with the segfault, but is
  // required to make the test semantically correct.
  barrier(CLK_LOCAL_MEM_FENCE);

  // This must be a loop of iteration length `size`. The use of 'size' needs to
  // be complex enough that it cannot be optimized out, this will result in a
  // memcpy being emitted.
  if (gid == 0) {
    for (size_t i = 0; i < size; i++) {
      out[i] = tmp[i];
    }
  }
}
