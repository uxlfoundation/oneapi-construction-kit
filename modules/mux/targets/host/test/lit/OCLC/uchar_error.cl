// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_scalar_uchar -compare dst,3,4,8,9 -arg src1,1,2,3,4 -arg "src2,range(0,3)" -arg scalar,2 -global 4 -local 4 -char-error 1 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_uchar(uchar scalar, __global uchar *src1, __global uchar*src2,
 __global uchar *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst - match
