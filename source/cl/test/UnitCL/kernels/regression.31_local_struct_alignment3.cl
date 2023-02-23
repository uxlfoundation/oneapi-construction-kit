// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

typedef struct {
  uchar a[7];
} s1;

typedef struct {
  float3 f;
} s2;

typedef struct {
  int2 i[13];
} s3;

typedef struct {
  uint4 i;
  long3 l[2];
  uchar *ptr;
} s4;

// Test struct of structs
typedef struct {
  s1 one;
  s2 two;
  s3 three;
  s4 four;

} __attribute__((aligned(ALIGN))) testStruct;

kernel void local_struct_alignment3(__global ulong *mask, __global ulong *out) {
  __local testStruct s[3];

  out[0] = (ulong)(&s[0]) & mask[0];
  out[1] = (ulong)(&s[1]) & mask[1];
  out[2] = (ulong)(&s[2]) & mask[2];
}
