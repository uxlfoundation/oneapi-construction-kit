// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vk/device.h>
#include <vk/image.h>

namespace vk {
VkResult CreateImage(vk::device device, const VkImageCreateInfo* pCreateInfo,
                     vk::allocator allocator, vk::image* pImage) {
  (void)device;
  (void)pCreateInfo;
  (void)allocator;
  (void)pImage;

  return VK_ERROR_FEATURE_NOT_PRESENT;
}

void DestroyImage(vk::device device, vk::image image, vk::allocator allocator) {
  (void)device;
  (void)image;
  (void)allocator;
}

void GetImageMemoryRequirements(vk::device device, vk::image image,
                                VkMemoryRequirements* pMemoryRequirements) {
  (void)device;
  (void)image;
  (void)pMemoryRequirements;
}

void GetImageSparseMemoryRequirements(
    vk::device device, vk::image image, uint32_t* pSparseMemoryAllocationCount,
    VkSparseImageMemoryRequirements* pSparseMemoryRequirements) {
  (void)device;
  (void)image;
  (void)pSparseMemoryAllocationCount;
  (void)pSparseMemoryRequirements;
}

void GetImageSubresourceLayout(vk::device device, vk::image image,
                               const VkImageSubresource* pSubresource,
                               VkSubresourceLayout* pLayout) {
  (void)device;
  (void)image;
  (void)pSubresource;
  (void)pLayout;
}
}  // namespace vk
