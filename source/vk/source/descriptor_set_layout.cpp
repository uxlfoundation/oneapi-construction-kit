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

#include <vk/descriptor_set_layout.h>
#include <vk/device.h>

namespace vk {
vk::descriptor_set_layout_t::descriptor_set_layout_t(vk::allocator allocator)
    : layout_bindings(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

vk::descriptor_set_layout_t::~descriptor_set_layout_t() {}

VkResult CreateDescriptorSetLayout(
    vk::device device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
    vk::allocator allocator, vk::descriptor_set_layout *pSetLayout) {
  (void)device;
  descriptor_set_layout descriptor_set_layout =
      allocator.create<vk::descriptor_set_layout_t>(
          VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, allocator);

  if (!descriptor_set_layout) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  if (descriptor_set_layout->layout_bindings.resize(
          pCreateInfo->bindingCount)) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  for (uint32_t bindingIndex = 0, bindingEnd = pCreateInfo->bindingCount;
       bindingIndex < bindingEnd; bindingIndex++) {
    descriptor_set_layout->layout_bindings[bindingIndex] =
        pCreateInfo->pBindings[bindingIndex];
  }

  *pSetLayout = descriptor_set_layout;

  return VK_SUCCESS;
}

void DestroyDescriptorSetLayout(vk::device device,
                                vk::descriptor_set_layout descriptorSetLayout,
                                vk::allocator allocator) {
  if (descriptorSetLayout == VK_NULL_HANDLE) {
    return;
  }

  (void)device;
  allocator.destroy<vk::descriptor_set_layout_t>(descriptorSetLayout);
}
}  // namespace vk
