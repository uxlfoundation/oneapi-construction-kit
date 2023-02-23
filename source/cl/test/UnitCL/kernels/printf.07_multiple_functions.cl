// Copyright (C) Codeplay Software Limited. All Rights Reserved.

void __attribute__((noinline)) g(void) { printf("%d,", 2); }

void __attribute__((noinline)) f(void) {
  printf("%d,", 1);
  g();
}

void __attribute__((noinline)) k(void) { printf("%d", 3); }

__kernel void multiple_functions(void) {
  f();
  k();
}
