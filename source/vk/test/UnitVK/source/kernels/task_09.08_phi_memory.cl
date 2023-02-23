// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void phi_memory(global int *input, global int *output, int size) {
  int gid = get_global_id(0);
  output = output + gid;
  for (int i = 0; i < size; i++) {
    *output = input[i + gid];
    output += 1;
  }
}
