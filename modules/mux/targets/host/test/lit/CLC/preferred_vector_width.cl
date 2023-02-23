// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %clc -d %host_ca_host_cl_device_name -cl-kernel-arg-info -cl-wfv=always -cl-llvm-stats -n -- %s 2> %t
// RUN: %filecheck < %t %s

__attribute__((reqd_work_group_size(16, 1, 1)))
__kernel void add(__global int* input1,
                  __global int* input2,
                  __global int* output) {
  int gid = get_local_id(0);
  output[gid] = input1[gid] + input2[gid];
}

// not going to check the number of vector instructions, since target-dependent subwidening
// doesn't necessarily result in 1 vector instruction per packetized instruction.
// CHECK: {{3|6|12}} vecz{{ *}}- Number of vector loads and stores in the vectorized kernel [ID#V0B]
// CHECK: 16 vecz{{ *}}- Vector width of the vectorized kernel [ID#V12]
// CHECK: 4 vecz-packetization{{ *}}- Number of instructions packetized [ID#P00]
