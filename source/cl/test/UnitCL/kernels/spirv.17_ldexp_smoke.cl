// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void ldexp_smoke(global float* out){
  size_t i = get_global_id(0);
  out[i] = ldexp(1.0f, 5);
}
