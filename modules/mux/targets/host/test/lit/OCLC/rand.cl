// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_scalar_float   -arg src1,rand(0.0,5.5),rand(-4.3,4.3),rand(-2.1,-0.01),4 -arg src2,repeat(4,rand(0,20.3)) -arg scalar,2 -print dst,4 -print src1,4 -print src2,4 -global 4 -local 4 -seed 2002 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_float(float scalar, __global float *src1, __global float*src2,
 __global float *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,16.40238,13.36131,19.91158,16.69472
// CHECK-NEXT: src1,0.4408496,3.247275,-0.7590057,4
// CHECK-NEXT: src2,13.96153,8.114035,18.67058,10.69472
