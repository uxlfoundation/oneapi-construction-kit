// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "kts/sub_group_helpers.h"

namespace kts {
namespace ucl {

bool mapSubGroupIds(
    size_t global_id, cl_uint2 sub_group_info,
    GlobalIdSubGroupGlobalIdMap &global_id_sub_group_global_id_map,
    SubGroupGlobalIdGlobalIdsMap &sub_group_global_id_global_ids_map) {
  auto sub_group_global_id = sub_group_info.x;
  auto sub_group_local_id = sub_group_info.y;
  if (sub_group_global_id >= sub_group_global_id_global_ids_map.size()) {
    return false;
  }
  if (sub_group_local_id >=
      sub_group_global_id_global_ids_map[sub_group_global_id].size()) {
    return false;
  }
  sub_group_global_id_global_ids_map[sub_group_global_id][sub_group_local_id] =
      global_id;
  global_id_sub_group_global_id_map[global_id] = sub_group_global_id;
  return true;
}

}  // namespace ucl
}  // namespace kts
