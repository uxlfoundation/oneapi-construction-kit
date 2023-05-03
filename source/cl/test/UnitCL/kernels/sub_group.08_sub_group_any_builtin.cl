// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
kernel void sub_group_any_builtin(global int *in, global size_t *out_a,
                                  global int *out_b) {
  const size_t glid = get_global_linear_id();
  const size_t sgid =
      get_sub_group_id() +
      get_enqueued_num_sub_groups() *
          (get_group_id(0) + get_group_id(1) * get_num_groups(0) +
           get_group_id(2) * get_num_groups(0) * get_num_groups(1));
  out_a[glid] = sgid;
  out_b[sgid] = sub_group_any(in[glid]);
}
