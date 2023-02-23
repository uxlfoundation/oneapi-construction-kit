// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: fp64
// RUN: %oclc -execute -enqueue vector_addition_scalar_double -arg src1,1.999,2,3,4 -arg src2,10,20.01,30,90 -arg scalar,2 -print dst,4 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_double(double scalar, __global double *src1, __global double*src2,
 __global double *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,13.999,24.01,35,96
