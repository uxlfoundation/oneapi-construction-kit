// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: fp64
// RUN: %oclc -execute -enqueue vector_addition_scalar_double -compare dst,2,4,6.6,8 -arg src1,1,2,3.600000000000001,4 -arg "src2,range(0,3)" -arg scalar,1 %s -ulp-error 2 > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_double(double scalar, __global double *src1, __global double*src2,
 __global double *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst - match
