// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <UnitVK.h>

// This is just a basic example test that is used for external inclusion in
// UnitVK. The goal of this is simply to test our ability to include external
// source files into the UnitVK framework.

class Host_VKExample : public uvk::RecordCommandBufferTest,
                       public uvk::DeviceMemoryTest {
 public:
  Host_VKExample()
      : DeviceMemoryTest(true),
        queueFamilyIndex(0),
        srcBuffer(VK_NULL_HANDLE),
        dstBuffer(VK_NULL_HANDLE),
        copy(),
        submitInfo() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(RecordCommandBufferTest::SetUp());

    vkGetDeviceQueue(device, 0, 0, &queue);

    bufferBytes = size * sizeof(uint32_t);

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
    DeviceMemoryTest::SetUp();

    vkBindBufferMemory(device, srcBuffer, memory, 0);
    vkBindBufferMemory(device, dstBuffer, memory, memoryBytes / 2);

    std::vector<uint32_t> data(size, size);

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

    // Skip all tests if we're running on a physical device that isn't the
    // Codeplay CPU. This check is at the end of SetUp(), to avoid complicating
    // the TearDown() function.
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    if (VK_VENDOR_ID_CODEPLAY != physicalDeviceProperties.vendorID ||
        VK_PHYSICAL_DEVICE_TYPE_CPU != physicalDeviceProperties.deviceType) {
      GTEST_SKIP();
    }
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
  const int size = 64;
};

TEST_F(Host_VKExample, Default) {
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copy);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(memoryBytes / 2, bufferBytes, &mappedMemory);

  for (int i = 0; i < size; i++) {
    ASSERT_EQ(64u, static_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(Host_VKExample, VendorID) {
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

  // 0x10004 is our vendor ID VK_VENDOR_ID_CODEPLAY
  ASSERT_EQ(0x10004, physicalDeviceProperties.vendorID);
  ASSERT_EQ(VK_VENDOR_ID_CODEPLAY, physicalDeviceProperties.vendorID);
}
