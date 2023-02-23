// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void work_dim(__global uint *out) {
  out[get_global_id(0)] = get_work_dim();
}
