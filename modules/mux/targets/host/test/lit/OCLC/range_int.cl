// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_scalar_long    -arg src1,range(9223372036854775807,9223372036854775804,-1) -arg src2,-1,-2,-3,-4 -arg scalar,1 -print dst,4 -print src1,4 -print src2,4 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_long(long scalar, __global long *src1, __global long*src2,
 __global long *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,9223372036854775807,9223372036854775805,9223372036854775803,9223372036854775801
// CHECK-NEXT: src1,9223372036854775807,9223372036854775806,9223372036854775805,9223372036854775804
// CHECK-NEXT: src2,-1,-2,-3,-4
