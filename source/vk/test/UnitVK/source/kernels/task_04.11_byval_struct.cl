// Copyright (C) Codeplay Software Limited. All Rights Reserved.

typedef struct _my_struct {
  int foo;
  int bar;
  int gee;
} my_struct;

void kernel byval_struct(__global int* in, my_struct my_str) {
  const int idx = get_global_id(0);
  in[idx] = (idx * my_str.foo) + (my_str.bar * my_str.gee);
}
