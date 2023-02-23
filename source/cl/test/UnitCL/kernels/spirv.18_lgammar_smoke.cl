// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void lgammar_smoke(global int* out){
  size_t i = get_global_id(0);
  lgamma_r(42.42f, out+i);
}
