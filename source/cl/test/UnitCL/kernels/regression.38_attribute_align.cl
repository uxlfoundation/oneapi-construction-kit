// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

typedef struct {
  char pad;
  short __attribute__((aligned(ALIGN1))) x;
} __attribute__((aligned(ALIGN2))) testStruct;

__constant testStruct constantStruct = {'a', 42};

kernel void attribute_align(__global ulong *mask, __global ulong *out) {
  __private testStruct privateStruct;
  __local testStruct localStruct;

  out[0] = ((ulong)&privateStruct.x) & mask[0];
  out[1] = ((ulong)&localStruct.x) & mask[1];
  out[2] = ((ulong)&constantStruct.x) & mask[2];

  out[3] = ((ulong)&privateStruct) & mask[3];
  out[4] = ((ulong)&localStruct) & mask[4];
  out[5] = ((ulong)&constantStruct) & mask[5];
}
