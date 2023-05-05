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

#ifndef VK_PIPELINE_LAYOUT_H_INCLUDED
#define VK_PIPELINE_LAYOUT_H_INCLUDED

#include <vk/allocator.h>
#include <vk/small_vector.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @copydoc ::vk::descriptor_set_layout_t
typedef struct descriptor_set_layout_t *descriptor_set_layout;

/// @brief internal pipeline_layout type
typedef struct pipeline_layout_t final {
  /// @brief Constructor
  ///
  /// @param allocator `vk::allocator` used to initialize
  /// `descriptorSetLayouts`
  /// @param total_push_constant_size size in bytes of the buffer to be created
  /// for push constants
  pipeline_layout_t(vk::allocator allocator, uint32_t total_push_constant_size);

  /// @brief Destructor
  ~pipeline_layout_t();

  /// @brief descriptor set layouts to be bound to the pipeline
  vk::small_vector<vk::descriptor_set_layout, 4> descriptor_set_layouts;

  /// @brief total size in bytes of the buffer needed for push constants
  uint32_t total_push_constant_size;
} * pipeline_layout;

/// @brief internal implementation of vkCreatePipelineLayout
///
/// @param device Device on which to create the pipeline layout
/// @param pCreateInfo create info
/// @param allocator allocator
/// @param pPipelineLayout return created pipeline layout
///
/// @return Return Vulkan result code
VkResult CreatePipelineLayout(vk::device device,
                              const VkPipelineLayoutCreateInfo *pCreateInfo,
                              vk::allocator allocator,
                              vk::pipeline_layout *pPipelineLayout);

/// @brief internal implementation of vkDestroyPipelineLayout
///
/// @param device Device on which the pipeline layout was created
/// @param pipelineLayout The pipeline layout to be destroyed
/// @param allocator Allocator used to created the pipeline layout
void DestroyPipelineLayout(vk::device device,
                           vk::pipeline_layout pipelineLayout,
                           vk::allocator allocator);
}  // namespace vk

#endif  // VK_PIPELINE_LAYOUT_H_INCLUDED
