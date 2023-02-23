// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// TODO: Enable offline, spir and spir-v testing (see CA-4062).
// REQUIRES: parameters
kernel void get_num_sub_groups(global uint *out) {
  out[get_global_linear_id()] = get_num_sub_groups();
}
