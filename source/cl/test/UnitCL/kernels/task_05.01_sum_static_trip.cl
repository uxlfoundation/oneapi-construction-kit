// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DKTS_DEFAULTS=ON"

#if defined(KTS_DEFAULTS)
#define TRIPS 256
#endif

__kernel void sum_static_trip(__global int *in1, __global int *in2,
                              __global int *out) {
  size_t tid = get_global_id(0);

  int sum = 0;
  for (int i = 0; i < TRIPS; i++) {
    sum += (in1[i] * i) + in2[i];
  }

  out[tid] = sum;
}
