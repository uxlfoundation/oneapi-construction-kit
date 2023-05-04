// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
kernel void get_num_sub_groups_builtin(global uint *out) {
  out[get_global_linear_id()] = get_num_sub_groups();
}
