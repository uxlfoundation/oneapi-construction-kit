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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkAllocateCommandBuffers

class AllocateCommandBuffers : public uvk::CommandPoolTest {
public:
  AllocateCommandBuffers() : allocateInfo(), commandBuffer(VK_NULL_HANDLE) {}

  virtual void SetUp() {
    RETURN_ON_FATAL_FAILURE(CommandPoolTest::SetUp());
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.commandBufferCount = 1;
  }

  virtual void TearDown() {
    if (commandBuffer) {
      vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    CommandPoolTest::TearDown();
  }

  VkCommandBufferAllocateInfo allocateInfo;
  VkCommandBuffer commandBuffer;
};

TEST_F(AllocateCommandBuffers, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateCommandBuffers(device, &allocateInfo,
                                                        &commandBuffer));
}

TEST_F(AllocateCommandBuffers, DefaultBufferLevelSecondary) {
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateCommandBuffers(device, &allocateInfo,
                                                        &commandBuffer));
}

TEST_F(AllocateCommandBuffers, ErrorOutOfHostMemory) {
  VkCommandPool nullPool;

  VkCommandPoolCreateInfo commandPoolCreateInfo = {};
  commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCreateInfo.queueFamilyIndex = 0;

  bool used = false;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateCommandPool(device, &commandPoolCreateInfo,
                                       uvk::oneUseAllocator(&used), &nullPool));

  allocateInfo.commandPool = nullPool;

  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));

  vkDestroyCommandPool(device, nullPool, uvk::oneUseAllocator(&used));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
