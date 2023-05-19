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

#include <vk/buffer.h>
#include <vk/buffer_view.h>
#include <vk/device.h>
#include <vk/type_traits.h>

namespace vk {
buffer_view_t::buffer_view_t(mux_buffer_t buffer, VkFormat format,
                             size_t offset, size_t range)
    : buffer(buffer), format(format), offset(offset), range(range) {}
buffer_view_t::~buffer_view_t() {}

VkResult CreateBufferView(vk::device device,
                          const VkBufferViewCreateInfo *pCreateInfo,
                          vk::allocator allocator, vk::buffer_view *pView) {
  (void)device;

  vk::buffer_view buffer_view = allocator.create<vk::buffer_view_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE,
      vk::cast<vk::buffer>(pCreateInfo->buffer)->mux_buffer,
      pCreateInfo->format, pCreateInfo->offset, pCreateInfo->range);

  if (!buffer_view) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  *pView = buffer_view;

  return VK_SUCCESS;
}

void DestroyBufferView(vk::device device, vk::buffer_view bufferView,
                       vk::allocator allocator) {
  if (bufferView == VK_NULL_HANDLE) {
    return;
  }

  (void)device;
  allocator.destroy(bufferView);
}
}  // namespace vk
