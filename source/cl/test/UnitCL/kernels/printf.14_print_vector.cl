// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void print_vector(global int4* in, int flag) {
  size_t gid = get_global_id(0);
  if (gid < flag) {
    printf("%d: %#04v4hi-%%-%#v4hlx\n", gid, in[gid], in[gid] - 1);
  }
}
