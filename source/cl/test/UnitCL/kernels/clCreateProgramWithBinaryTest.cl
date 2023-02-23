// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Offline-compiled kernel used with clCreateProgramWithBinaryTest
kernel void foo(global int* in_buf, global int* out_buf) {
  int idx = get_global_id(0);
  out_buf[idx] = in_buf[idx];
}
