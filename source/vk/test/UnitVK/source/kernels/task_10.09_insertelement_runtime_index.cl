// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void insertelement_runtime_index(global int4* in, global int4* out,
                                        global int* index) {
  size_t gid = get_global_id(0);

  out[gid] = in[gid];
  out[gid][index[gid]] = 42;
}
