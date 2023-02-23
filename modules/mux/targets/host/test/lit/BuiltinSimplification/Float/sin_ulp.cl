// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc %s -execute -enqueue sin_fold -ulp-error 4 \
// RUN: -compare scalar_out,0.8414709848078965 \
// RUN: -compare vector_out,0.8414709848078965,-0.9165215479156338 \
// RUN: > %t
// RUN: %filecheck < %t %s

void kernel sin_fold(global float* scalar_out,
                     global float2* vector_out) {
  *scalar_out = sin(1.0f);
  *vector_out = sin((float2)(1.0f, 42.0f));
}

// CHECK: scalar_out - match
// CHECK: vector_out - match
