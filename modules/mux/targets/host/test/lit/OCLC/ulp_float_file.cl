// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_scalar_float -compare dst:%p/Inputs/compare-file-ulp.txt -arg src1,1,2,3.6000001,4 -arg "src2,range(0,3)" -arg scalar,2 -global 4 -local 4 -ulp-error 1 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_float(float scalar, __global float *src1, __global float*src2,
 __global float *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst - match
