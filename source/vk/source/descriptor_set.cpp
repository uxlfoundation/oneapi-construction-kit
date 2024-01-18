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

#include <vk/buffer.h>
#include <vk/buffer_view.h>
#include <vk/descriptor_pool.h>
#include <vk/descriptor_set.h>
#include <vk/descriptor_set_layout.h>
#include <vk/device.h>
#include <vk/type_traits.h>
#include <vk/unique_ptr.h>

namespace vk {
descriptor_set_t::descriptor_set_t(vk::allocator allocator)
    : descriptor_bindings(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

descriptor_set_t::~descriptor_set_t() {}

VkResult AllocateDescriptorSets(
    vk::device device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
    VkDescriptorSet *pDescriptorSets) {
  (void)device;

  vk::descriptor_pool descriptor_pool =
      vk::cast<vk::descriptor_pool>(pAllocateInfo->descriptorPool);

  VK_ASSERT(
      pAllocateInfo->descriptorSetCount <= descriptor_pool->remaining_sets,
      "No descriptor sets remaining in this pool!");

  const vk::allocator allocator = descriptor_pool->allocator;

  for (int descriptorSetIndex = 0,
           decriptorSetEnd = pAllocateInfo->descriptorSetCount;
       descriptorSetIndex < decriptorSetEnd; descriptorSetIndex++) {
    vk::descriptor_set descriptor_set = allocator.create<vk::descriptor_set_t>(
        VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, allocator);

    if (!descriptor_set) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    vk::unique_ptr<vk::descriptor_set> descriptor_set_ptr(descriptor_set,
                                                          {allocator});

    vk::descriptor_set_layout descriptor_set_layout =
        vk::cast<vk::descriptor_set_layout>(
            pAllocateInfo->pSetLayouts[descriptorSetIndex]);

    if (descriptor_set->descriptor_bindings.resize(
            descriptor_set_layout->layout_bindings.size())) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    for (const VkDescriptorSetLayoutBinding &layout_binding :
         descriptor_set_layout->layout_bindings) {
      mux_descriptor_info_t *descriptors =
          static_cast<mux_descriptor_info_t *>(allocator.alloc(
              layout_binding.descriptorCount * sizeof(mux_descriptor_info_t),
              VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE));

      if (!descriptors) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }

      auto binding = allocator.create<vk::descriptor_binding_t>(
          VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, layout_binding.descriptorCount,
          descriptors);

      if (!binding) {
        allocator.free(descriptors);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }

      descriptor_set->descriptor_bindings[layout_binding.binding] = binding;
    }

    if (descriptor_pool->descriptor_sets.push_back(descriptor_set_ptr.get())) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    descriptor_pool->remaining_sets--;
    pDescriptorSets[descriptorSetIndex] =
        reinterpret_cast<VkDescriptorSet>(descriptor_set_ptr.release());
  }

  return VK_SUCCESS;
}

VkResult FreeDescriptorSets(vk::device device,
                            vk::descriptor_pool descriptorPool,
                            uint32_t descriptorSetCount,
                            const vk::descriptor_set *pDescriptorSets) {
  (void)device;
  for (uint32_t descriptorSetIndex = 0; descriptorSetIndex < descriptorSetCount;
       descriptorSetIndex++) {
    if (pDescriptorSets[descriptorSetIndex] == VK_NULL_HANDLE) {
      continue;
    }

    for (vk::descriptor_binding binding :
         pDescriptorSets[descriptorSetIndex]->descriptor_bindings) {
      descriptorPool->allocator.free(binding->descriptors);
      descriptorPool->allocator.destroy(binding);
    }
    descriptorPool->allocator.destroy(pDescriptorSets[descriptorSetIndex]);
    descriptorPool->descriptor_sets.erase(
        std::remove(descriptorPool->descriptor_sets.begin(),
                    descriptorPool->descriptor_sets.end(),
                    pDescriptorSets[descriptorSetIndex]),
        descriptorPool->descriptor_sets.end());
    descriptorPool->remaining_sets++;
  }
  return VK_SUCCESS;
}

void UpdateDescriptorSets(vk::device device, uint32_t descriptorWriteCount,
                          const VkWriteDescriptorSet *pDescriptorWrites,
                          uint32_t descriptorCopyCount,
                          const VkCopyDescriptorSet *pDescriptorCopies) {
  (void)device;

  for (uint32_t writeIndex = 0; writeIndex < descriptorWriteCount;
       writeIndex++) {
    const VkWriteDescriptorSet write = pDescriptorWrites[writeIndex];

    vk::descriptor_set descriptor_set =
        vk::cast<vk::descriptor_set>(write.dstSet);

    if (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC == write.descriptorType ||
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC == write.descriptorType) {
      descriptor_set->descriptor_bindings[write.dstBinding]->dynamic = true;
    }

    uint32_t dstBinding = write.dstBinding;
    const uint32_t descriptorCount = write.descriptorCount;
    uint32_t dstArrayElement = write.dstArrayElement;

    // used to index the descriptor we are writing to independently from the
    // descriptor info, allows for the former to be reset in the event of an
    // overflow
    uint32_t descriptorIndex = 0;

    for (uint32_t descriptorInfoIndex = 0;
         descriptorInfoIndex < descriptorCount; descriptorInfoIndex++) {
      // check if we need to roll over to the next binding
      if (dstArrayElement + descriptorInfoIndex + 1 >
          descriptor_set->descriptor_bindings[dstBinding]->descriptor_count) {
        dstArrayElement = 0;
        dstBinding += 1;

        descriptorIndex = 0;
      }

      mux_descriptor_info_t *descriptor =
          &descriptor_set->descriptor_bindings[dstBinding]
               ->descriptors[dstArrayElement + descriptorIndex];
      descriptorIndex++;

      switch (write.descriptorType) {
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
          descriptor->type = mux_descriptor_info_type_buffer;
          descriptor->buffer_descriptor.buffer =
              vk::cast<vk::buffer>(
                  write.pBufferInfo[descriptorInfoIndex].buffer)
                  ->mux_buffer;
          descriptor->buffer_descriptor.offset =
              write.pBufferInfo[descriptorInfoIndex].offset;
          break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
          descriptor->type = mux_descriptor_info_type_buffer;
          descriptor->buffer_descriptor.buffer =
              vk::cast<vk::buffer_view>(
                  write.pTexelBufferView[descriptorInfoIndex])
                  ->buffer;
          descriptor->buffer_descriptor.offset =
              vk::cast<vk::buffer_view>(
                  write.pTexelBufferView[descriptorInfoIndex])
                  ->offset;
          break;
        default:
          break;
      }
    }
  }

  for (uint32_t copyIndex = 0; copyIndex < descriptorCopyCount; copyIndex++) {
    const VkCopyDescriptorSet copy = pDescriptorCopies[copyIndex];

    vk::descriptor_set dst_set = vk::cast<vk::descriptor_set>(copy.dstSet);

    vk::descriptor_set src_set = vk::cast<vk::descriptor_set>(copy.srcSet);

    const uint32_t descriptorCount = copy.descriptorCount;

    uint32_t dstBinding = copy.dstBinding;
    uint32_t dstArrayElement = copy.dstArrayElement;
    uint32_t srcBinding = copy.srcBinding;
    uint32_t srcArrayElement = copy.srcArrayElement;

    // here we need to be able to keep track of how many descriptors we're
    // copying, index the src_set descriptors and index the dst_set descriptors
    // independently
    uint32_t srcIndex = 0;
    uint32_t dstIndex = 0;

    for (uint32_t descriptorIndex = 0; descriptorIndex < descriptorCount;
         descriptorIndex++) {
      // check if we need to roll over to the next dst binding
      if (dstArrayElement + descriptorIndex + 1 >
          dst_set->descriptor_bindings[dstBinding]->descriptor_count) {
        dstArrayElement = 0;
        dstBinding += 1;
        dstIndex = 0;
      }
      // check if we need to roll over to the next src binding
      if (srcArrayElement + descriptorIndex + 1 >
          src_set->descriptor_bindings[srcBinding]->descriptor_count) {
        srcArrayElement = 0;
        srcBinding += 1;
        srcIndex = 0;
      }
      dst_set->descriptor_bindings[dstBinding]
          ->descriptors[dstArrayElement + dstIndex] =
          src_set->descriptor_bindings[srcBinding]
              ->descriptors[srcArrayElement + srcIndex];
      dstIndex++;
      srcIndex++;
    }
  }
}
}  // namespace vk
