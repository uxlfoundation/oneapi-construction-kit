// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DREAD_LOCAL_SIZE=4";"-DGLOBAL_ID=1"

__kernel void barrier_with_ifs(__global int* output) {
  int local_id = get_local_id(0);
  __local int cache[READ_LOCAL_SIZE];

  int output_val = 0;

  for (int d = 0; d < get_global_size(0); d++) {
    cache[local_id] = local_id;

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_global_id(0) == GLOBAL_ID) {
      // This is intentional as the bug occurs in the else.
      if (get_local_id(0) != get_local_id(0)) {
        output_val = cache[d] - 1;
      } else {
        // The contents of this if don't matter hence why its
        // the same as one of the proceeding ones.
        if (get_global_id(0) == GLOBAL_ID) {
          // Unrolled loop since a for loop here hides the bug
          // only the first cache[d] access is required
          output_val += cache[0];
          output_val += cache[1];
          output_val += cache[2];
          output_val += cache[3];
        }
      }
    }
  }

  if (get_global_id(0) == GLOBAL_ID) {
    output[0] = output_val;
  }
}
