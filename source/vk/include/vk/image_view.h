// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
