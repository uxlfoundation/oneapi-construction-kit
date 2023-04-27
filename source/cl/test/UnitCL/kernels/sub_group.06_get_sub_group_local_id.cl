// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CLC OPTIONS: -cl-std=CL3.0
kernel void get_sub_group_local_id(global uint *out) {
  out[get_global_linear_id()] = get_sub_group_local_id();
}
