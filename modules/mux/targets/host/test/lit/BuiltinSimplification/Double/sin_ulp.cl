// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc %s -execute -enqueue sin_fold -ulp-error 4 \
// RUN: -compare scalar_out,0.8414709848078965 \
// RUN: -compare vector_out,0.8414709848078965,0.9092974268256817 \
// RUN: > %t
// RUN: %filecheck < %t %s

void kernel sin_fold(global double* scalar_out,
                     global double2* vector_out) {
  *scalar_out = sin(1.0);
  *vector_out = sin((double2)(1.0, 2.0));
}

// CHECK: scalar_out - match
// CHECK: vector_out - match
