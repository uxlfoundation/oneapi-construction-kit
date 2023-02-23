// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void phi_memory2(global int *input, global int *output, int size) {
  int gid = get_global_id(0);
  output = output + gid;
  if (gid & 5) goto scalar;
  if (size > 2672688) goto scalar;

  for (int i = 0; i < size; i++) {
    *output = input[i + gid];
    output += 1;
  }
  if (size < 6248288) return;

scalar:
  for (int i = 0; i < size; i++) {
    *output = input[i + gid];
    output += 1;
  }
}
