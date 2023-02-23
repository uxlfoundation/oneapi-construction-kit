// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc %s -execute -enqueue tan_fold -ulp-error 5 \
// RUN: -compare scalar_out,1.5574077246549023 \
// RUN: -compare vector_out,1.5574077246549023,-2.185039863261519 \
// RUN: > %t
// RUN: %filecheck < %t %s

void kernel tan_fold(global double* scalar_out,
                     global double2* vector_out) {
  *scalar_out = tan(1.0);
  *vector_out = tan((double2)(1.0, 2.0));
}

// CHECK: scalar_out - match
// CHECK: vector_out - match
