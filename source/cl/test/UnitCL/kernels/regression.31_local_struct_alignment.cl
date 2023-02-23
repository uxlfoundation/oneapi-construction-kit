// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

typedef struct {
  char c;
  short vec[3];
  int4 i4;
#ifdef NO_DOUBLES
  long l;
#else
  double d;
#endif
} __attribute__((aligned(ALIGN))) testStruct;

kernel void local_struct_alignment(__global ulong *mask, __global ulong *out) {
  __local testStruct s[3];

  out[0] = (ulong)(&s[0]) & mask[0];
  out[1] = (ulong)(&s[1]) & mask[1];
  out[2] = (ulong)(&s[2]) & mask[2];
}
