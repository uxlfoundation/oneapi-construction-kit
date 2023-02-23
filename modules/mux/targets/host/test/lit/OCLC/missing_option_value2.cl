// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: debug
// RUN: %not %oclc -cl-options "--dummy-host-option --dummy-host-flag" %s 2> %t
// RUN: %filecheck < %t %s

// CHECK: Build program failed with error: CL_INVALID_BUILD_OPTIONS (-43)

__kernel void add(__global int* input1,
                  __global int* input2,
                  __global int* output) {
  int gid = get_local_id(0);
  output[gid] = input1[gid] + input2[gid];
}
