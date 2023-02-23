// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_scalar_float    -arg src1,range(0,3) -arg src2,range(-0.666363,-0.666414,-0.000017) -arg scalar,2 -print dst,4 -print src1,4 -print src2,4 -global 4 -local 4 %s >& %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_float(float scalar, __global float *src1, __global float*src2,
 __global float *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,1.333637,2.33362,3.333603,4.333586
// CHECK-NEXT: src1,0,1,2,3
// CHECK-NEXT: src2,-0.666363,-0.66638,-0.666397,-0.666414
