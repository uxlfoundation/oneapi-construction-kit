// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// DEFINITIONS: "-DGROUP_RANGE_1D=2";"-DLOCAL_ITEMS_1D=2"

__kernel void group_barrier_4(__global int* group_info) {
  int groupId0 = get_group_id(0);
  int groupId1 = get_group_id(1);
  int groupIdL = (groupId1 * GROUP_RANGE_1D) + groupId0;
  int lid = (get_local_id(1) * LOCAL_ITEMS_1D) + get_local_id(0);
  barrier(CLK_LOCAL_MEM_FENCE);
  if (lid == 0) {
    group_info[groupIdL] = 7;
  }
};
