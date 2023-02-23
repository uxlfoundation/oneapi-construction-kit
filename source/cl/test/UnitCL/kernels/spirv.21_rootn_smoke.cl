// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void rootn_smoke(global float* out){
  size_t i = get_global_id(0);
  out[i] = rootn(42.42f, 1);
}
