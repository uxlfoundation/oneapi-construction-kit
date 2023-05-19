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

#include <vk/device.h>
#include <vk/query_pool.h>

namespace vk {
VkResult CreateQueryPool(vk::device device,
                         const VkQueryPoolCreateInfo* pCreateInfo,
                         vk::allocator allocator, vk::query_pool* pQueryPool) {
  (void)device;
  (void)pCreateInfo;
  (void)allocator;
  (void)pQueryPool;

  return VK_ERROR_FEATURE_NOT_PRESENT;
}

void DestroyQueryPool(vk::device device, vk::query_pool queryPool,
                      vk::allocator allocator) {
  (void)device;
  (void)queryPool;
  (void)allocator;
}

VkResult GetQueryPoolResults(vk::device device, vk::query_pool queryPool,
                             uint32_t firstQuery, uint32_t queryCount,
                             size_t dataSize, void* pData, VkDeviceSize stride,
                             VkQueryResultFlags flags) {
  (void)device;
  (void)queryPool;
  (void)firstQuery;
  (void)queryCount;
  (void)dataSize;
  (void)pData;
  (void)stride;
  (void)flags;

  return VK_ERROR_FEATURE_NOT_PRESENT;
}
}  // namespace vk
