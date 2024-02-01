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

#ifndef VK_ERROR_H_INCLUDED
#define VK_ERROR_H_INCLUDED

#include <compiler/result.h>
#include <mux/mux.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

/// @brief Display MESSAGE then abort due to catastrophic failure.
///
/// @param MESSAGE Message to display.
#define VK_ABORT(MESSAGE)                                              \
  {                                                                    \
    (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, MESSAGE); \
    abort();                                                           \
  }

/// @brief Check CONDITION and display MESSAGE then abort due to exceptional
/// failure.
///
/// @param CONDITION Condition that must be true.
/// @param MESSAGE Message to display when condition is false.
#define VK_ASSERT(CONDITION, MESSAGE) \
  {                                   \
    if (!(CONDITION)) {               \
      VK_ABORT(MESSAGE);              \
    }                                 \
  }

namespace vk {

/// @brief Function to translate mux_result values into VkResults
///
/// @param error Mux result to translate
VkResult getVkResult(mux_result_t error);

/// @brief Function to translate compiler result values into VkResults
///
/// @param error Mux compiler result to translate
VkResult getVkResult(compiler::Result error);

}  // namespace vk

#endif  // VK_ERROR_H_INCLUDED
