// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_scalar_int    -arg src1,randint(0,5),randint(-4,4),randint(-61,-1),4 -arg src2,repeat(4,randint(0,20)) -arg scalar,2 -print dst,4 -print src1,4 -print src2,4 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_int(int scalar, __global int *src1, __global int*src2,
 __global int *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,25,1,-7,11
// CHECK-NEXT: src1,4,-1,-17,4
// CHECK-NEXT: src2,19,0,8,5
