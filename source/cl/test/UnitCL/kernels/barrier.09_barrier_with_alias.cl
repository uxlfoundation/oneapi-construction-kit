// Copyright (C) Codeplay Software Limited. All Rights Reserved.

struct mystruct {
  int x;
  int y;
  int z;
};

struct mystruct2 {
  int* ptr;
};

__kernel void barrier_with_alias(__global int* output) {
  int global_id = get_global_id(0);

  struct mystruct foo;

  foo.x = 20;
  foo.y = 2;
  foo.z = 3;

  struct mystruct2 foo2;
  foo2.ptr = &foo.x;  // -> 20

  barrier(CLK_LOCAL_MEM_FENCE);

  struct mystruct foo3;

  if (get_global_id(0) == 0) {
    foo3.x = get_global_id(0) + 10;
  }
  foo.x = 1;  // *foo2.ptr should now become 1

  output[global_id] = *foo2.ptr /* 1 */ + foo.z /* 3 */ + global_id;
}
