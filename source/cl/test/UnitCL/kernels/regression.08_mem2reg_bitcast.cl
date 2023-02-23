// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// SPIR OPTIONS: "-w"
// SPIRV OPTIONS: "-w"

__kernel void mem2reg_bitcast(__global int *result) {
  uchar l_270[8] = {};
  uint ui2 = l_270[0] || 1;
  result[get_global_id(0)] = ui2;
}
