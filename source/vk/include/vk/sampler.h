// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_SAMPLER_H_INCLUDED
#define VK_SAMPLER_H_INCLUDED

#include <vk/allocator.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t* device;

/// @brief Stub of internal sampler type
typedef struct sampler_t final {
} * sampler;

/// @brief Stub of vkCreateSampler
VkResult CreateSampler(vk::device device,
                       const VkSamplerCreateInfo* pCreateInfo,
                       vk::allocator allocator, vk::sampler* pSampler);

/// @brief Stub of vkDestroySampler
void DestroySampler(vk::device device, vk::sampler sampler,
                    vk::allocator allocator);
}  // namespace vk

#endif
