// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void order(int active) {
  int x = get_global_id(0);
  if (x == active) printf("Execution %d\n", x);
}
