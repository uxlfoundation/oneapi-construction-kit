// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void primitive(__global float *out, float val) {
  out[get_global_id(0)] = val;
}
