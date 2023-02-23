// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half parameters
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_private(__global half *input,
                           uint iterations,
                           __global half *output) {
  size_t gid = get_global_id(0);
  HALFN global_copy = LOADN(gid, input);  // vload from __global

  __private half private_array[ARRAY_LEN];
  for (uint i = 0; i < iterations; i++) {
    STOREN(global_copy, i, private_array);  // vstore to __private
  }

  bool identical = true;
  for (uint i = 0; i < iterations; i++) {
    HALFN private_copy = LOADN(i, private_array);  // vload from __private
    bool are_equal = all(isequal(global_copy, private_copy) ||
                         (isnan(global_copy) && isnan(private_copy)));
    identical = identical && are_equal;
  }

  if (identical) {
    STOREN(global_copy, gid, output);  // vstore to __global
  }
};
