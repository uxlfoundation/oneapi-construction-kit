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
#include <vk/pipeline_layout.h>
#include <vk/type_traits.h>

#include <numeric>

namespace vk {
pipeline_layout_t::pipeline_layout_t(vk::allocator allocator,
                                     uint32_t total_push_constant_size)
    : descriptor_set_layouts(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}),
      total_push_constant_size(total_push_constant_size) {}

pipeline_layout_t::~pipeline_layout_t() {}

VkResult CreatePipelineLayout(vk::device device,
                              const VkPipelineLayoutCreateInfo *pCreateInfo,
                              vk::allocator allocator,
                              vk::pipeline_layout *pPipelineLayout) {
  (void)device;

  // TODO: replace std::accumulate with cargo::accumulate when it is implemented
  const uint32_t push_constant_buffer_size = std::accumulate(
      pCreateInfo->pPushConstantRanges,
      pCreateInfo->pPushConstantRanges + pCreateInfo->pushConstantRangeCount, 0,
      [](const uint32_t &current,
         const VkPushConstantRange &range) -> uint32_t {
        (void)current;
        return range.size;
      });

  vk::pipeline_layout pipeline_layout_out =
      allocator.create<pipeline_layout_t>(VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE,
                                          allocator, push_constant_buffer_size);
  if (!pipeline_layout_out) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  if (pipeline_layout_out->descriptor_set_layouts.resize(
          pCreateInfo->setLayoutCount)) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  for (int descriptorSetIndex = 0,
           descriptorSetEnd = pCreateInfo->setLayoutCount;
       descriptorSetIndex < descriptorSetEnd; descriptorSetIndex++) {
    pipeline_layout_out->descriptor_set_layouts[descriptorSetIndex] =
        vk::cast<vk::descriptor_set_layout>(
            pCreateInfo->pSetLayouts[descriptorSetIndex]);
  }

  *pPipelineLayout = pipeline_layout_out;

  return VK_SUCCESS;
}

void DestroyPipelineLayout(vk::device device,
                           vk::pipeline_layout pipelineLayout,
                           vk::allocator allocator) {
  if (pipelineLayout == VK_NULL_HANDLE) {
    return;
  }

  (void)device;
  allocator.destroy(pipelineLayout);
}
}  // namespace vk
