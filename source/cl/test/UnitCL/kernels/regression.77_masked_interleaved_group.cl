// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void masked_interleaved_group(__global char *out, __global char *in) {
  int x = get_global_id(0) * 2;
  out[x] = 0;
  out[x + 1] = 0;

  // the barrier is to ensure the non-written values are initialised to zeroes,
  // but while ensuring we retain the masked stores in the second part.
  barrier(CLK_GLOBAL_MEM_FENCE);

  __global char *base_in = in + x;
  char v1 = base_in[0];
  char v2 = base_in[1];

  __global char *base_out = out + x;
  if (v1 + v2 < 0) {
    base_out[1] = v1;
  } else {
    base_out[0] = v2;
  }
}
