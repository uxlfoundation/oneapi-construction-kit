// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: debug
// RUN: %not %clc -d %host_ca_host_cl_device_name -n --dummy-host-option -- %s 2> %t
// RUN: %filecheck < %t %s

// CHECK: error: failed to parse command line arguments.

__kernel void add(__global int* input1,
                  __global int* input2,
                  __global int* output) {
  int gid = get_local_id(0);
  output[gid] = input1[gid] + input2[gid];
}
