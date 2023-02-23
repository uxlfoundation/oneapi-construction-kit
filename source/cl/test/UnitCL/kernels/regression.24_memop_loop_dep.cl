// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void memop_loop_dep(global int *in, global int *out, int i, int e) {
  size_t gid = get_global_id(0);
  // i = 0, e = 1, for one iteration
  for (; i < e; i++) {
    int4 in4 = vload4(gid, in);
    vstore4(in4, gid, out);
    // this will never be executed, but at the stage that we run vecz it will
    // not be optimized away either
    if (in4.s0 && !gid) {
      while (gid)
        ;
    }
  }
}
