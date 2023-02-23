// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void pown_smoke(global float* out){
  size_t i = get_global_id(0);
  out[i] = pown(0.5f, 2);
}
