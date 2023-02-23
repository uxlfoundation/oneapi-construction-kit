// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vk/device.h>
#include <vk/sampler.h>

namespace vk {
VkResult CreateSampler(vk::device device,
                       const VkSamplerCreateInfo* pCreateInfo,
                       vk::allocator allocator, vk::sampler* pSampler) {
  (void)device;
  (void)pCreateInfo;
  (void)allocator;
  (void)pSampler;

  return VK_ERROR_FEATURE_NOT_PRESENT;
}

void DestroySampler(vk::device device, vk::sampler sampler,
                    vk::allocator allocator) {
  (void)device;
  (void)sampler;
  (void)allocator;
}
}  // namespace vk
