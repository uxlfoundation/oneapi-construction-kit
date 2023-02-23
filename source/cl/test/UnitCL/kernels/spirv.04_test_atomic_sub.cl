// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void test_atomic_sub(volatile __global uint* in) {
  size_t index = get_global_id(0);
  atomic_sub(in + index, 1);
}
