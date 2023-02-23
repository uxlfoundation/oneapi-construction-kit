// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void test_atomic_add(volatile __global uint* in) {
  size_t index = get_global_id(0);
  atomic_add(in + index, 1);
}
