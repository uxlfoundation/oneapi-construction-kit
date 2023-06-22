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

#include <vk/descriptor_pool.h>
#include <vk/descriptor_set.h>
#include <vk/device.h>

namespace vk {
descriptor_pool_t::descriptor_pool_t(
    uint32_t max_sets, VkDescriptorPoolCreateFlags create_flag_bits,
    vk::allocator allocator)
    : max_sets(max_sets),
      remaining_sets(max_sets),
      create_flag_bits(create_flag_bits),
      allocator(allocator),
      descriptor_sets(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

descriptor_pool_t::~descriptor_pool_t() {}

VkResult CreateDescriptorPool(vk::device device,
                              const VkDescriptorPoolCreateInfo *pCreateInfo,
                              vk::allocator allocator,
                              vk::descriptor_pool *pDescriptorPool) {
  (void)device;

  // TODO: a better solution than storing the allocator used to create the
  // command pool
  vk::descriptor_pool descriptor_pool = allocator.create<vk::descriptor_pool_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_OBJECT, pCreateInfo->maxSets,
      pCreateInfo->flags, allocator);

  if (!descriptor_pool) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  *pDescriptorPool = descriptor_pool;

  return VK_SUCCESS;
}

void DestroyDescriptorPool(vk::device device,
                           vk::descriptor_pool descriptorPool,
                           vk::allocator allocator) {
  if (descriptorPool == VK_NULL_HANDLE) {
    return;
  }

  (void)device;

  for (vk::descriptor_set descriptor_set : descriptorPool->descriptor_sets) {
    for (vk::descriptor_binding binding : descriptor_set->descriptor_bindings) {
      descriptorPool->allocator.free(binding->descriptors);
      descriptorPool->allocator.destroy(binding);
    }
    descriptorPool->allocator.destroy(descriptor_set);
  }

  allocator.destroy<vk::descriptor_pool_t>(descriptorPool);
}

VkResult ResetDescriptorPool(vk::device device,
                             vk::descriptor_pool descriptorPool,
                             VkDescriptorPoolResetFlags flags) {
  // flags is reserved for future use
  (void)flags;
  (void)device;

  for (vk::descriptor_set descriptor_set : descriptorPool->descriptor_sets) {
    for (vk::descriptor_binding binding : descriptor_set->descriptor_bindings) {
      descriptorPool->allocator.free(binding->descriptors);
      descriptorPool->allocator.destroy(binding);
    }
    descriptorPool->allocator.destroy(descriptor_set);
  }
  descriptorPool->descriptor_sets.clear();

  descriptorPool->remaining_sets = descriptorPool->max_sets;

  return VK_SUCCESS;
}
}  // namespace vk
