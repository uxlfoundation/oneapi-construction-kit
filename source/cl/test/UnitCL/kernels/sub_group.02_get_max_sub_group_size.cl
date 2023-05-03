// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
kernel void get_max_sub_group_size(global uint *out) {
  out[get_global_linear_id()] = get_max_sub_group_size();
}
