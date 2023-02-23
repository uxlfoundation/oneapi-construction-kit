// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DKTS_DEFAULTS=ON"

#if defined(KTS_DEFAULTS)
#define TRIPS 256
#endif

__kernel void sum_static_trip_uniform(__global int *in1, __global int *in2,
                                      __global int *out) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);

  int sum = 0;
  for (int i = 0; i < TRIPS; i++) {
    int p = i + lid;
    sum += (in1[p] * i) + in2[p];
  }

  out[tid] = sum;
}
