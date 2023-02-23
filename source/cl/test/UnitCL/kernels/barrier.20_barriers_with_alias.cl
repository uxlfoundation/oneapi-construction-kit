// Copyright (C) Codeplay Software Limited. All Rights Reserved.

struct mystruct {
  int x;
};

struct mystruct2 {
  int* ptr;
};

__kernel void barriers_with_alias(__global int* output) {
  int global_id = get_global_id(0);

  struct mystruct foo;

  foo.x = 20;

  struct mystruct2 foo2;
  if (global_id & 1) {
    foo.x += 2;
    foo2.ptr = &foo.x;  // -> 22
  } else {
    foo2.ptr = &foo.x;  // -> 20
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  output[global_id] = **&foo2.ptr + global_id;
}
