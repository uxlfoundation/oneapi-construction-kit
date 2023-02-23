// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void frexp_smoke(global int* out){
  size_t i = get_global_id(0);
  frexp(42.42f, out+i);
}
