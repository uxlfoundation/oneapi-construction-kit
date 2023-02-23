// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -cl-options "-cl-opt-disable" -execute -enqueue vector_multiply_local -arg src,1,2,3,4,5,6,7,8,9,10 -arg tmp,40 -print dst,10 -global 10 -local 2 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_multiply_local(__global int *src, __local int *tmp, __global int *dst) {
    size_t gid = get_global_id(0);
    size_t lid = get_local_id(0);
    tmp[lid] = src[gid] * 2;
    dst[gid] = tmp[lid] * 2;
}

// CHECK: dst,4,8,12,16,20,24,28,32,36,40
