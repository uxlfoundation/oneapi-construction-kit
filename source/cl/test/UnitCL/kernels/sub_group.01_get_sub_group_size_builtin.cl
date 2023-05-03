// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
kernel void get_sub_group_size_builtin(global uint *out) {
  out[get_global_linear_id()] = get_sub_group_size();
}
