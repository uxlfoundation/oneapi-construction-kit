// Copyright (C) Codeplay Software Limited. All Rights Reserved.

struct struct_needs_32bit_align {
  char x;
  int y;
};

__kernel void barrier_store_mask(int mask, __global int* output) {
  int global_id = get_global_id(0);

  struct struct_needs_32bit_align needs_32bit_align;

  needs_32bit_align.y = global_id + 12;

  barrier(CLK_LOCAL_MEM_FENCE);
  int num_out = 2;
  // use xor as testing for zeroes can lead to false positives
  output[global_id * num_out] = (((int)&needs_32bit_align.y) ^ mask) & mask;
  output[global_id * num_out + 1] = needs_32bit_align.y;
}
