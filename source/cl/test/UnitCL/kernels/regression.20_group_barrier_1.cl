// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
