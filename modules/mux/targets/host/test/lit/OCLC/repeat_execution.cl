// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// FIXME: DISABLED (See CA-1147)
// UNSUPPORTED: true

// RUN: %oclc -execute -enqueue vector_addition_scalar_int    -arg src1,repeat(3,{repeat(4,randint(-100,100))}) -arg src2,10,20,30,40 -arg scalar,{1},{2},{3} -global {32,1,1},{64,4,1},{128,4,1} -local {16,1,1},{32,2,1},{64,4,1} -print dst,4 -repeat-execution 3 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_int(int scalar, __global int *src1, __global int*src2,
 __global int *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,69,-28,73,131
// CHECK-NEXT: dst,-84,4,-17,-53
// CHECK-NEXT: dst,17,-7,-11,55
