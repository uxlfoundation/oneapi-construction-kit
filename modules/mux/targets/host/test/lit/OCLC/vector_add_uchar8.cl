// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_vec_uchar8 -arg src1,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8 -arg src2,repeat(32,randint(0,15)) -arg vec,100,200,100,200,100,100,100,100 -print dst,32 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_vec_uchar8(uchar8 vec, __global uchar8 *src1, __global uchar8* src2,
 __global uchar8 *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = (uchar8) vec + src1[gid] + src2[gid];
}

// CHECK: dst,113,206,114,219,105,112,111,108,109,207,107,212,107,114,115,121,108,208,114,207,108,111,121,110,104,205,114,219,117,107,114,112
