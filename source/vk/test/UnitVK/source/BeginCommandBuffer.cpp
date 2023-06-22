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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkBeginCommandBuffer

class BeginCommandBuffer : public uvk::CommandPoolTest {
 public:
  BeginCommandBuffer() : commandBuffer(VK_NULL_HANDLE), beginInfo() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(CommandPoolTest::SetUp());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  }

  virtual void TearDown() override {
    if (commandBuffer) {
      vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    CommandPoolTest::TearDown();
  }

  VkCommandBuffer commandBuffer;
  VkCommandBufferBeginInfo beginInfo;
};

TEST_F(BeginCommandBuffer, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkBeginCommandBuffer(commandBuffer, &beginInfo));
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
