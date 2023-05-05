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

#ifndef VK_IMAGE_VIEW_H_INCLUDED
#define VK_IMAGE_VIEW_H_INCLUDED

#include <vk/allocator.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t* device;

/// @brief Stub of internal image_view type
typedef struct image_view_t final {
} * image_view;

/// @brief Stub of vkCreateImageView
VkResult CreateImageView(vk::device device,
                         const VkImageViewCreateInfo* pCreateInfo,
                         vk::allocator allocator, vk::image_view* pView);

/// @brief Stub of vkDestroyImageView
void DestroyImageView(vk::device device, vk::image_view imageView,
                      vk::allocator allocator);
}  // namespace vk

#endif
