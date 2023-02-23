// Copyright (C) Codeplay Software Limited. All Rights Reserved.

typedef struct {
  char a;
  short3 b;
  uchar2 offset1;
  int* c;
  short offset2;
  float d[3];
  char offset3;
  long4 e;
} UserStruct;

kernel void struct_param_alignment(__global UserStruct* my_struct,
                                   __global ulong* mask, __global ulong* out) {
  out[0] = ((ulong)&my_struct[0].b) & mask[0];
  out[1] = ((ulong)&my_struct[1].c) & mask[1];
  out[2] = ((ulong)&my_struct[2].d[0]) & mask[2];
  out[3] = ((ulong)&my_struct[3].d[1]) & mask[3];
  out[4] = ((ulong)&my_struct[4].d[2]) & mask[4];
  out[5] = ((ulong)&my_struct[5].e) & mask[5];
}
