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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCmdCopyBuffer

class CmdCopyBuffer : public uvk::RecordCommandBufferTest,
                      public uvk::DeviceMemoryTest {
 public:
  CmdCopyBuffer()
      : DeviceMemoryTest(true),
        queueFamilyIndex(0),
        srcBuffer(VK_NULL_HANDLE),
        dstBuffer(VK_NULL_HANDLE),
        copy(),
        submitInfo() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(RecordCommandBufferTest::SetUp());

    vkGetDeviceQueue(device, 0, 0, &queue);

    bufferBytes = 64 * sizeof(uint32_t);

    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.size = bufferBytes;
    createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &queueFamilyIndex;

    vkCreateBuffer(device, &createInfo, nullptr, &srcBuffer);
    vkCreateBuffer(device, &createInfo, nullptr, &dstBuffer);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, srcBuffer, &memoryRequirements);

    memoryBytes = memoryRequirements.size * 2;

    DeviceMemoryTest::memorySize = memoryBytes;
    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    vkBindBufferMemory(device, srcBuffer, memory, 0);
    vkBindBufferMemory(device, dstBuffer, memory, memoryBytes / 2);

    std::vector<uint32_t> data(64, 64);

    void *mappedMemory;

    DeviceMemoryTest::mapMemory(0, bufferBytes, &mappedMemory);
    memcpy(mappedMemory, data.data(), bufferBytes);
    DeviceMemoryTest::unmapMemory();

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    copy.size = bufferBytes;
    copy.dstOffset = 0;
    copy.srcOffset = 0;
  }

  virtual void TearDown() override {
    vkDestroyBuffer(device, srcBuffer, nullptr);
    vkDestroyBuffer(device, dstBuffer, nullptr);

    DeviceMemoryTest::TearDown();
    RecordCommandBufferTest::TearDown();
  }

  uint32_t memoryBytes;
  uint32_t bufferBytes;
  uint32_t queueFamilyIndex;
  VkQueue queue;
  VkBuffer srcBuffer, dstBuffer;
  VkBufferCopy copy;
  VkSubmitInfo submitInfo;
};

TEST_F(CmdCopyBuffer, Default) {
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copy);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(memoryBytes / 2, bufferBytes, &mappedMemory);

  for (int i = 0; i < 64; i++) {
    ASSERT_EQ(64u, static_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdCopyBuffer, DefaultSecondaryCommandBuffer) {
  VkCommandBufferAllocateInfo allocInf = {};
  allocInf.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInf.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  allocInf.commandPool = commandPool;
  allocInf.commandBufferCount = 1;

  VkCommandBuffer secondaryCommandBuffer;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateCommandBuffers(
                                   device, &allocInf, &secondaryCommandBuffer));

  VkCommandBufferInheritanceInfo inheritInfo = {};
  inheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
  inheritInfo.framebuffer = VK_NULL_HANDLE;
  inheritInfo.occlusionQueryEnable = VK_FALSE;

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = &inheritInfo;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkBeginCommandBuffer(secondaryCommandBuffer, &beginInfo));
  vkCmdCopyBuffer(secondaryCommandBuffer, srcBuffer, dstBuffer, 1, &copy);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(secondaryCommandBuffer));

  vkCmdExecuteCommands(commandBuffer, 1, &secondaryCommandBuffer);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(memoryBytes / 2, bufferBytes, &mappedMemory);

  for (int i = 0; i < 64; i++) {
    ASSERT_EQ(64u, static_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
  vkFreeCommandBuffers(device, commandPool, 1, &secondaryCommandBuffer);
}

