// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc %s -execute -enqueue cos_fold -ulp-error 4 \
// RUN: -compare scalar_out,0.5403023058681398 \
// RUN: -compare vector_out,0.5403023058681398,-0.39998531498835127 \
// RUN: > %t
// RUN: %filecheck < %t %s

void kernel cos_fold(global float* scalar_out,
                     global float2* vector_out) {
  *scalar_out = cos(1.0f);
  *vector_out = cos((float2)(1.0f, 42.0f));
}

// CHECK: scalar_out - match
// CHECK: vector_out - match
