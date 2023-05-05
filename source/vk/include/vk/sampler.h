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
