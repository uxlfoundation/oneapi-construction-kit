// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void dont_mask_workitem_builtins(__constant int* in,
                                          __global int* out) {
  int lid = get_local_id(0);

  if (lid > 0) {
    int gid = get_global_id(0);
    out[gid] = in[gid];
  } else {
    // don't use get_global_id again, to prevent the compiler for lifting
    // them outside the if/else block
    int gid = lid + (get_local_size(0) * get_group_id(0));
    out[gid] = 42;
  }
}
