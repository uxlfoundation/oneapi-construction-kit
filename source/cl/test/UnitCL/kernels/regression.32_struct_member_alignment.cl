// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

struct node {
  struct node* ptr;
  struct node*** triple_ptr;
  short offset1;
  int16 x;
};

typedef struct {
  uchar2 offset2;
  struct node* linked_list;
} nestedStruct;

typedef struct {
  char offset3;
  TYPE m;
  nestedStruct struct_array[3][2];
} testStruct;

kernel void struct_member_alignment(__global ulong* mask, __global ulong* out) {
  __private testStruct s1[2];
  __private nestedStruct s2;
  __private struct node s3;

  out[0] = ((ulong)&s1[0].m) & mask[0];
  out[1] = ((ulong)&s2.linked_list) & mask[1];
  out[2] = ((ulong)&s3.x) & mask[2];
}
