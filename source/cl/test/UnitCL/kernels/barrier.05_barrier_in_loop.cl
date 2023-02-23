// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DREAD_LOCAL_SIZE=16";"-DOUTER_LOOP_SIZE=1";"-DGLOBAL_ID=0"

__kernel void barrier_in_loop(__global int* output) {
  int global_id = get_global_id(0);
  int local_id = get_local_id(0);
  __local int cache[READ_LOCAL_SIZE];

  int output_val = 0;

  for (int d = 0; d < OUTER_LOOP_SIZE; d++) {
    cache[local_id] = local_id;
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int i = 0; i < READ_LOCAL_SIZE; i++) {
      output_val += cache[i];
    }
  }

  if (global_id == GLOBAL_ID) {
    output[0] = output_val;
  }
}
