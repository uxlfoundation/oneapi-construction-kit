// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// DEFINITIONS: "-DGROUP_RANGE_1D=2";"-DGROUP_RANGE_2D=2"

__kernel void group_barrier_1(__global int4* group_info) {
  int groupId0 = get_group_id(0);
  int groupId1 = get_group_id(1);
  int groupId2 = get_group_id(2);
  int groupIdL = (groupId2 * GROUP_RANGE_2D * GROUP_RANGE_1D) +
                 (groupId1 * GROUP_RANGE_1D) + groupId0;
  barrier(CLK_LOCAL_MEM_FENCE);
  group_info[groupIdL] = (int4)(groupId0, groupId1, groupId2, groupIdL);
};
