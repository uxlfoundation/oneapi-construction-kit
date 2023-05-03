// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: nospirv
__kernel void side_effects(__global int* s) {
  int i = 0;
  printf("%lc", 1, ++i);
  printf("%d", 1, ++i);
  *s = i;
}
