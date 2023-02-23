// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void insertelement_constant_index(global int4* in, global int4* out) {
  size_t gid = get_global_id(0);

  out[gid] = in[gid];
  out[gid][2] = 42;
}
