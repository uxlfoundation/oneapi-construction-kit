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

#ifndef VK_QUERY_POOL_H_INCLUDED
#define VK_QUERY_POOL_H_INCLUDED

#include <vk/allocator.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @brief Stub of internal query_pool type
typedef struct query_pool_t final {
} * query_pool;

/// @brief Stub of vkCreateQueryPool
VkResult CreateQueryPool(vk::device device,
                         const VkQueryPoolCreateInfo *pCreateInfo,
                         vk::allocator allocator, vk::query_pool *pQueryPool);

/// @brief Stub of vkDestroyQueryPool
void DestroyQueryPool(vk::device device, vk::query_pool queryPool,
                      vk::allocator allocator);

/// @brief Stub of vkGetQueryPoolResults
VkResult GetQueryPoolResults(vk::device device, vk::query_pool queryPool,
                             uint32_t firstQuery, uint32_t queryCount,
                             size_t dataSize, void *pData, VkDeviceSize stride,
                             VkQueryResultFlags flags);
}  // namespace vk

#endif
