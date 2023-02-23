// Copyright (C) Codeplay Software Limited. All Rights Reserved.

typedef struct {
  char a[3];
  short2 b;
  uchar3 offset1;
  ulong c;
  int offset2;
  float3 d;
} testStruct;

__constant testStruct bar = {{'a', 'b', 'c'}, {1, 2}, {'x', 'y', 'z'}, 42u, 42,
                             {0.9, 0.8, 0.7}};

kernel void constant_struct_alignment(__global ulong* mask,
                                      __global ulong* out) {
  out[0] = ((ulong)&bar.b) & mask[0];
  out[1] = ((ulong)&bar.c) & mask[1];
  out[2] = ((ulong)&bar.d) & mask[2];
}
