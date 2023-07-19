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

#ifndef VK_DESCRIPTOR_SET_LAYOUT_H_INCLUDED
#define VK_DESCRIPTOR_SET_LAYOUT_H_INCLUDED

#include <vk/allocator.h>
#include <vk/small_vector.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @brief internal descriptor_set_layout type
typedef struct descriptor_set_layout_t final {
  /// @brief Constructor
  ///
  /// @param allocator `vk::allocator` used to initialize `layout_bindings`
  descriptor_set_layout_t(vk::allocator allocator);

  /// @brief Destructor
  ~descriptor_set_layout_t();

  /// @brief List of layout bindings
  vk::small_vector<VkDescriptorSetLayoutBinding, 4> layout_bindings;
} *descriptor_set_layout;

/// @brief Internal implementation of vkCreateDescriptorSetLayout
///
/// @param device Device that the layout will be used on
/// @param pCreateInfo create info
/// @param allocator allocator
/// @param pSetLayout return created descriptor_set_layout
///
/// @return Return Vulkan result code
VkResult CreateDescriptorSetLayout(
    vk::device device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
    vk::allocator allocator, vk::descriptor_set_layout *pSetLayout);

/// @brief Internal implementation of vkDestroyDescriptorSetLayout
///
/// @param device Device on which the descriptor set layout was created
/// @param descriptorSetLayout descriptor set layout to destroy
/// @param allocator Allocator that was used to created the descriptorSetLayout
void DestroyDescriptorSetLayout(vk::device device,
                                vk::descriptor_set_layout descriptorSetLayout,
                                vk::allocator allocator);
}  // namespace vk

#endif  // VK_DESCRIPTOR_SET_LAYOUT_H_INCLUDED
