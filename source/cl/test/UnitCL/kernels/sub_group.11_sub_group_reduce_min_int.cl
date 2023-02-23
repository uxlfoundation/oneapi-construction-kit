// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// TODO: Enable offline, spir and spir-v testing (see CA-4062).
// REQUIRES: parameters
kernel void sub_group_reduce_min_int(global int *in, global size_t *out_a,
                                     global int *out_b) {
  const size_t glid = get_global_linear_id();
  const size_t sgid =
      get_sub_group_id() +
      get_enqueued_num_sub_groups() *
          (get_group_id(0) + get_group_id(1) * get_num_groups(0) +
           get_group_id(2) * get_num_groups(0) * get_num_groups(1));
  out_a[glid] = sgid;
  out_b[sgid] = sub_group_reduce_min(in[glid]);
}
