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

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateBuffer

class CreateBuffer : public uvk::DeviceTest {
 public:
  CreateBuffer() : createInfo(), buffer(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    queueFamily = 0;

    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.size = 16;
    createInfo.pQueueFamilyIndices = &queueFamily;
    createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }

  virtual void TearDown() override {
    if (buffer) {
      vkDestroyBuffer(device, buffer, nullptr);
    }
    DeviceTest::TearDown();
  }

  uint32_t queueFamily;
  VkBufferCreateInfo createInfo;
  VkBuffer buffer;
};

TEST_F(CreateBuffer, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateBuffer(device, &createInfo, nullptr, &buffer));
}

TEST_F(CreateBuffer, DefaultAllocator) {
  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreateBuffer(device, &createInfo, uvk::defaultAllocator(), &buffer));
  vkDestroyBuffer(device, buffer, uvk::defaultAllocator());
  buffer = VK_NULL_HANDLE;
}

TEST_F(CreateBuffer, DefaultSharingModeConcurrent) {
  uint32_t qfms[2] = {0, 0};

  createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
  createInfo.queueFamilyIndexCount = 2;
  createInfo.pQueueFamilyIndices = qfms;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateBuffer(device, &createInfo, nullptr, &buffer));
}

TEST_F(CreateBuffer, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateBuffer(device, &createInfo, uvk::nullAllocator(), &buffer));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
