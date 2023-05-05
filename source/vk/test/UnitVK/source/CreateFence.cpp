

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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateFence

class CreateFence : public uvk::DeviceTest {
public:
  CreateFence() : fence(VK_NULL_HANDLE), createInfo() {}

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  }

  void TearDown() override {
    if (fence) {
      vkDestroyFence(device, fence, nullptr);
    }
    DeviceTest::TearDown();
  }

  VkFence fence;
  VkFenceCreateInfo createInfo;
};

TEST_F(CreateFence, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateFence(device, &createInfo, nullptr, &fence));
}

TEST_F(CreateFence, DefaultFlagsSignaled) {
  createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateFence(device, &createInfo, nullptr, &fence));
  ASSERT_EQ_RESULT(vkGetFenceStatus(device, fence), VK_SUCCESS);
}

TEST_F(CreateFence, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateFence(device, &createInfo,
                                             uvk::defaultAllocator(), &fence));

  vkDestroyFence(device, fence, uvk::defaultAllocator());
  fence = VK_NULL_HANDLE;
}

TEST_F(CreateFence, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateFence(device, &createInfo, uvk::nullAllocator(), &fence));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable due to the fact
// that we can't currently access device memory  allocators to mess with.
