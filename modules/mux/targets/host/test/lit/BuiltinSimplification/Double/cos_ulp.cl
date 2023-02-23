// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc %s -execute -enqueue cos_fold -ulp-error 4 \
// RUN: -compare scalar_out,0.5403023058681398 \
// RUN: -compare vector_out,0.5403023058681398,-0.4161468365471424 \
// RUN: > %t
// RUN: %filecheck < %t %s

void kernel cos_fold(global double* scalar_out,
                     global double2* vector_out) {
  *scalar_out = cos(1.0);
  *vector_out = cos((double2)(1.0, 2.0));
}

// CHECK: scalar_out - match
// CHECK: vector_out - match
