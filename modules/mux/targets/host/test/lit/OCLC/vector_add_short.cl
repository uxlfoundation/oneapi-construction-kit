// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_scalar_short  -arg src1,1,2,3,4 -arg src2,10,20,30,90 -arg scalar,2 -print dst,4 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_short(short scalar, __global short *src1, __global short*src2,
 __global short *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,13,24,35,96
