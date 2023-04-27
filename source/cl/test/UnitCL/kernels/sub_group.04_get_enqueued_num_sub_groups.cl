// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CLC OPTIONS: -cl-std=CL3.0
kernel void get_enqueued_num_sub_groups(global uint *out) {
  out[get_global_linear_id()] = get_enqueued_num_sub_groups();
}
