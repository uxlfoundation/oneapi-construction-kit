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

#ifndef VK_IMAGE_H_INCLUDED
#define VK_IMAGE_H_INCLUDED

#include <vk/allocator.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t* device;

/// @brief Stub of internal image type
typedef struct image_t final {
} * image;

/// @brief Stub of vkCreateImage
VkResult CreateImage(vk::device device, const VkImageCreateInfo* pCreateInfo,
                     vk::allocator allocator, vk::image* pImage);

/// @brief Stub of vkDestroyImage
void DestroyImage(vk::device device, vk::image image, vk::allocator allocator);

/// @brief Stub of vkGetImageMemoryRequirements
void GetImageMemoryRequirements(vk::device device, vk::image image,
                                VkMemoryRequirements* pMemoryRequirements);

/// @brief Stub of vkGetImageSparseMemoryRequirements
void GetImageSparseMemoryRequirements(
    vk::device device, vk::image image, uint32_t* pSparseMemoryAllocationCount,
    VkSparseImageMemoryRequirements* pSparseMemoryRequirements);

/// @brief Stub of vkGetImageSubresourceLayout
void GetImageSubresourceLayout(vk::device device, vk::image image,
                               const VkImageSubresource* pSubresource,
                               VkSubresourceLayout* pLayout);
}  // namespace vk

#endif
