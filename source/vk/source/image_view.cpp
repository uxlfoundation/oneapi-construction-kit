// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vk/device.h>
#include <vk/image_view.h>

namespace vk {
VkResult CreateImageView(vk::device device,
                         const VkImageViewCreateInfo* pCreateInfo,
                         vk::allocator allocator, vk::image_view* pView) {
  (void)device;
  (void)pCreateInfo;
  (void)allocator;
  (void)pView;
  return VK_ERROR_FEATURE_NOT_PRESENT;
}

void DestroyImageView(vk::device device, vk::image_view imageView,
                      vk::allocator allocator) {
  (void)device;
  (void)imageView;
  (void)allocator;
}
}  // namespace vk
