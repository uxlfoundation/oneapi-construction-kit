// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void first_kernel(void) {
  printf("Hello first_kernel %d %s \n", 3, "Bar");
}

__kernel void multiple_kernels(void) {
  printf("Hello multiple_kernels %s %d\n", "Foo", 10);
}
