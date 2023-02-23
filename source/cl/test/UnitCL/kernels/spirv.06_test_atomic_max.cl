// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void test_atomic_max(__global uint* in, __global uint* out){
  size_t index = get_global_id(0);
  atomic_max(out + index, in[index]);
}
