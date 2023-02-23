// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Offline-compiled kernel used with clCreateBufferHostPtr
void __kernel add_floats(__global float *A, __global float *B,
                         __global float *C) {
  size_t i = get_global_id(0);
  C[i] = A[i] + B[i];
}
