// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: generic-target
// RUN: %oclc -cl-options "-cl-vec=slp" -enqueue slp_test -stage vectorized %s > %t
// RUN: %filecheck < %t %s

__kernel void slp_test(__global int *out, __global int *in1, __global int *in2) {
  int x = get_global_id(0) * 8;

  int a0 = in1[x + 0] + in2[x + 0];
  int a1 = in1[x + 1] + in2[x + 1];
  int a2 = in1[x + 2] + in2[x + 2];
  int a3 = in1[x + 3] + in2[x + 3];
  int a4 = in1[x + 4] + in2[x + 4];
  int a5 = in1[x + 5] + in2[x + 5];
  int a6 = in1[x + 6] + in2[x + 6];
  int a7 = in1[x + 7] + in2[x + 7];

  out[x + 0] = a0;
  out[x + 1] = a1;
  out[x + 2] = a2;
  out[x + 3] = a3;
  out[x + 4] = a4;
  out[x + 5] = a5;
  out[x + 6] = a6;
  out[x + 7] = a7;
}

// it makes sure that the prevectorization option creates two vector adds

// CHECK: add nsw <4 x i32>
// CHECK: add nsw <4 x i32>
