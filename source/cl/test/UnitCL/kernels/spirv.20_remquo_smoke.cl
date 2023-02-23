// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void remquo_smoke(global int* out){
  size_t i = get_global_id(0);
  remquo(42.42f, 2.0f, out+i);
}
