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
#ifndef UNITCL_KTS_SUB_GROUPS_H_INCLUDED
#define UNITCL_KTS_SUB_GROUPS_H_INCLUDED

#include <CL/cl.h>

#include <vector>

namespace kts {
namespace ucl {

using GlobalIdSubGroupGlobalIdMap = std::vector<size_t>;
using SubGroupGlobalIdGlobalIdsMap = std::vector<std::vector<size_t>>;

/// @brief Maps work-item global IDs to/from unique sub-group IDs.
/// @param global_id The global ID for this work-item
/// @param sub_group_info A two-element vector of IDs: X is the sub-group global
/// ID, Y is sub-group local ID.
/// @param global_id_sub_group_global_id_map A map from global ID to sub-group
/// global ID. Will be populated by this method.
/// @param sub_group_global_id_global_ids_map A map from sub-group global ID and
/// sub-group local ID to the work-item local ID. Will be populated by this
/// method, but must be pre-allocated for every sub-group in the ND range.
/// @return true on success, false on failure
bool mapSubGroupIds(
    size_t global_id, cl_uint2 sub_group_info,
    GlobalIdSubGroupGlobalIdMap &global_id_sub_group_global_id_map,
    SubGroupGlobalIdGlobalIdsMap &sub_group_global_id_global_ids_map);

}  // namespace ucl
}  // namespace kts
#endif  // UNITCL_KTS_SUB_GROUPS_H_INCLUDED
