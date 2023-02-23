// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// DEFINITIONS: "-DGROUP_RANGE_1D=2";"-DGROUP_RANGE_2D=2";"-DLOCAL_ITEMS_1D=4";"-DLOCAL_ITEMS_2D=2"

__kernel void group_barrier_3(__global int8* group_info) {
  int groupId0 = get_group_id(0);
  int groupId1 = get_group_id(1);
  int groupId2 = get_group_id(2);
  int groupIdL = (groupId2 * GROUP_RANGE_2D * GROUP_RANGE_1D) +
                 (groupId1 * GROUP_RANGE_1D) + groupId0;
  __local int tmp[4];
  int lid = (get_local_id(2) * LOCAL_ITEMS_2D * LOCAL_ITEMS_1D) +
            (get_local_id(1) * LOCAL_ITEMS_1D) + get_local_id(0);
  if (lid == 0) {
    tmp[0] = groupId0;
    tmp[1] = groupId1;
    tmp[2] = groupId2;
    tmp[3] = groupIdL;
  }
  barrier(CLK_LOCAL_MEM_FENCE);
  if (lid == 0) {
    group_info[groupIdL] = (int8)(groupId0, groupId1, groupId2, groupIdL,
                                  tmp[0], tmp[1], tmp[2], tmp[3]);
  }
};
