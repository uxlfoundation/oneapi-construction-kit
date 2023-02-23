// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void atomic_cmpxchg_builtin(volatile global int *counter,
                                   volatile global int *out) {
  int tid = get_global_id(0);
  int c = -2;

  do {
    c = atomic_cmpxchg(counter, tid - 1, tid);
  } while (c != tid - 1);

  out[tid] = c;
}
