// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void private_array(global int *in, global int *out) {
  size_t tid = get_global_id(0);
 private
  int array[16];
  for (int i = 0; i < 16; i++) {
    array[i] = in[i];
  }

  int sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += array[i];
  }
  out[tid] = sum;
}
