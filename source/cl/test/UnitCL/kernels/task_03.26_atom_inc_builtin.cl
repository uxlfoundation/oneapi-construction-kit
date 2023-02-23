// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void atom_inc_builtin(volatile global int *counter,
                             volatile global int *out) {
  size_t tid = get_global_id(0);

  out[tid] = atom_inc(counter);
}
