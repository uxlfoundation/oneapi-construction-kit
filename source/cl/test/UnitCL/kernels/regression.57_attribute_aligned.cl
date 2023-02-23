// Copyright (C) Codeplay Software Limited. All Rights Reserved.

struct struct_needs_1024byte_align {
  char c;
  short vec[3];
  int4 i4;
  long l;
} __attribute__((aligned(1024)));

__kernel void attribute_aligned(int mask, __global int* output) {
  int global_id = get_global_id(0);

  struct struct_needs_1024byte_align needs_1024byte_align;

  needs_1024byte_align.l = 0xdeafbeef & global_id;

  int num_out = 2;
  // use xor as testing for zeroes can lead to false positives
  output[global_id * num_out] = (((int)&needs_1024byte_align) ^ mask) & mask;
  output[global_id * num_out + 1] = needs_1024byte_align.l;
}
