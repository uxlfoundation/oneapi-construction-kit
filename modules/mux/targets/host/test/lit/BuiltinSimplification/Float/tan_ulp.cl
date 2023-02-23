// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc %s -execute -enqueue tan_fold -ulp-error 5 \
// RUN: -compare scalar_out,1.5574077246549023 \
// RUN: -compare vector_out,1.5574077246549023,2.2913879924374863 \
// RUN: > %t
// RUN: %filecheck < %t %s

void kernel tan_fold(global float* scalar_out,
                     global float2* vector_out) {
  *scalar_out = tan(1.0f);
  *vector_out = tan((float2)(1.0f, 42.0f));
}

// CHECK: scalar_out - match
// CHECK: vector_out - match
