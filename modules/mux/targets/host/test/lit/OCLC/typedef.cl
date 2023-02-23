// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_scalar_real  -arg src1,1,2,3,4 -arg src2,10,20,30.4,90.6 -arg scalar,2 -print dst,4 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

typedef float REAL;
__kernel void vector_addition_scalar_real(__global REAL *src1, __global REAL *src2,
                                          __global REAL *dst, REAL scalar) {
    size_t gid = get_global_id(0);
    dst[gid] = src1[gid] + src2[gid] + scalar;
}

// CHECK: dst,13,24,35.4,96.6
