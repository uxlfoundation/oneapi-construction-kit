// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// DEFINITIONS: -D NUM_INPUTS=16

#ifndef NUM_INPUTS
#define NUM_INPUTS 1
#endif

kernel void floats_vectors(global float2* in, global float2* out) {
  size_t gid = get_global_id(0);
  out[gid] = in[gid] * in[gid];
  int i;
  // We are only printing from one workitem to make sure the order remains
  // constant
  if (gid == 0) {
    for (i = 0; i < NUM_INPUTS; ++i) {
      printf("%#16.1v2hlA\n", in[i] * in[i]);
    }
  }
}
