// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void test_atomic_compare_exchange(__global uint* in){
  size_t index = get_global_id(0);
  atomic_cmpxchg(in + index, 0, 42);
}
