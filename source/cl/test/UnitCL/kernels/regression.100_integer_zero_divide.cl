// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// SPIRV OPTIONS: "-Wno-division-by-zero"
// Old clang emits a compile error instead of a warning for divide by zero.
// REQUIRES: nospir

__kernel void integer_zero_divide(__global int* out) {
  size_t tid = get_global_id(0);
  size_t tidZeroDiv = tid / 0;
  if (tidZeroDiv == 0 || tidZeroDiv != 0) {
    out[tid] = tid;
  }
}
