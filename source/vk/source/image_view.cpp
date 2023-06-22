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

#include <vk/device.h>
#include <vk/image_view.h>

namespace vk {
VkResult CreateImageView(vk::device device,
                         const VkImageViewCreateInfo *pCreateInfo,
                         vk::allocator allocator, vk::image_view *pView) {
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
