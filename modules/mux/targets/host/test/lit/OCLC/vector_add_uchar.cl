// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_scalar_uchar  -arg src1,1,2,3,4 -arg src2,10,20,30,90 -arg scalar,2 -print dst,4 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_uchar(uchar scalar, __global uchar *src1, __global uchar*src2,
 __global uchar *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,13,24,35,96
