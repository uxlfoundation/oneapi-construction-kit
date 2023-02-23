// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void multiple_printf(void) {
  printf("1,%d,", 2);
  printf("3,%d,", 4);
  printf("5,%d", 6);
}
