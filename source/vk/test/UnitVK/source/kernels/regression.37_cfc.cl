// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#define CHUNK_SIZE 32

kernel void cfc(global const int* in, global int* out, int limit) {
  size_t x = get_global_id(0);

  int temp[CHUNK_SIZE];
  for (int i = 0; i < CHUNK_SIZE; ++i) {
    temp[i] = in[i];
  }

  if (x < limit) {
    out[x] = x;
  } else {
    out[x] = temp[x % CHUNK_SIZE];
  }
}
