// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: debug
// RUN: env CA_LLVM_OPTIONS=-print-after=reduce-to-func \
// RUN: %clc -d %host_ca_host_cl_device_name -n --dummy-host-flag --dummy-host-flag2 -- %s 2> %t
// RUN: %filecheck < %t %s

__kernel void add(__global int* input1,
                  __global int* input2,
                  __global int* output) {
  int gid = get_local_id(0);
  output[gid] = input1[gid] + input2[gid];
}

// CHECK: !host.build_options = !{[[MD_STRING:![0-9]+]]}
// CHECK: [[MD_STRING]] = !{!"--dummy-host-flag,;--dummy-host-flag2,"}
