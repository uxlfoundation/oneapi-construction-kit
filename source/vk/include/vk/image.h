// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
