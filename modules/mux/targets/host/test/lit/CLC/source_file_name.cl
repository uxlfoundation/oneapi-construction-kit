// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: rm -f %t
// RUN: %clc -d %host_ca_host_cl_device_name -S%t -n -- %s
// RUN: %filecheck < %t %s

__kernel void nothing() {}

// CHECK: __kernel void nothing() {}
