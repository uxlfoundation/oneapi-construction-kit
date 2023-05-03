// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
kernel void sub_group_scan_exclusive_add_int(global int *in,
                                             global uint2 *out_a,
                                             global int *out_b) {
  const size_t glid = get_global_linear_id();
  const size_t sgid =
      get_sub_group_id() +
      get_enqueued_num_sub_groups() *
          (get_group_id(0) + get_group_id(1) * get_num_groups(0) +
           get_group_id(2) * get_num_groups(0) * get_num_groups(1));
  out_a[glid].x = sgid;
  out_a[glid].y = get_sub_group_local_id();
  out_b[glid] = sub_group_scan_exclusive_add(in[glid]);
}
