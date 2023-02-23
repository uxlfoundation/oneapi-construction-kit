// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

// Test array of vec types
typedef struct {
  uchar u;
  short3 s[2];
  float f;
  int2 c[4][4];
} __attribute__((aligned(ALIGN))) testStruct;

kernel void local_struct_alignment2(__global ulong *mask, __global ulong *out) {
  __local testStruct s[3];

  out[0] = (ulong)(&s[0]) & mask[0];
  out[1] = (ulong)(&s[1]) & mask[1];
  out[2] = (ulong)(&s[2]) & mask[2];
}
