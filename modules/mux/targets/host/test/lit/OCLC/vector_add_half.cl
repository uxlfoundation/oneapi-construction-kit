// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_half          -arg src1,1,2,3,4 -arg src2,10,20,30,90               -print dst,4 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_half(__global half *src1, __global half *src2,
 __global half *dst) {
	size_t gid = get_global_id(0);
	float f1 = vload_half(gid, src1);
	float f2 = vload_half(gid, src2);
	float fd = f1 + f2;
	vstore_half(fd, gid, dst);
}

// CHECK: 11,22,33,94
