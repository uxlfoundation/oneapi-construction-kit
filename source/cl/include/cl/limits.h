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

/// @file
///
/// @brief OpenCL specified limits.

#ifndef CL_LIMITS_H_INCLUDED
#define CL_LIMITS_H_INCLUDED

#include <cstddef>
#include <cstdint>

namespace cl {
/// @addtogroup cl
/// @{

namespace max {
/// @brief  Define the limits of the work item, work group.
enum limits : uint32_t {
  WORK_ITEM_DIM = 3,  ///< Maximum supported work item dimensions.
};
}  // namespace max

/// @}
}  // namespace cl

#endif  // CL_LIMITS_H_INCLUDED
