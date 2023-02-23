// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCmdFillBuffer

class CmdFillBuffer : public uvk::RecordCommandBufferTest,
                      public uvk::BufferTest,
                      public uvk::DeviceMemoryTest {
 public:
  CmdFillBuffer()
      : BufferTest(64, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   true),
        DeviceMemoryTest(true),
        submitInfo() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(RecordCommandBufferTest::SetUp());
    vkGetDeviceQueue(device, 0, 0, &queue);

    RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

    DeviceMemoryTest::memorySize = BufferTest::bufferMemoryRequirements.size;
    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    vkBindBufferMemory(device, buffer, memory, 0);

    std::vector<uint32_t> data(16, 23);
    void *mappedMemory;

    DeviceMemoryTest::mapMemory(0, 64, &mappedMemory);
    memcpy(mappedMemory, data.data(), 64);
    DeviceMemoryTest::unmapMemory();

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
  }

  virtual void TearDown() override {
    BufferTest::TearDown();
    DeviceMemoryTest::TearDown();
    RecordCommandBufferTest::TearDown();
  }

  VkQueue queue;
  VkSubmitInfo submitInfo;
};

TEST_F(CmdFillBuffer, Default) {
  vkCmdFillBuffer(commandBuffer, buffer, 0, 64, 32);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;
  DeviceMemoryTest::mapMemory(0, 64, &mappedMemory);
  for (int dataIndex = 0; dataIndex < 16; dataIndex++) {
    ASSERT_EQ(32u, static_cast<uint32_t *>(mappedMemory)[dataIndex]);
  }
  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdFillBuffer, DefaultSecondaryCommandBuffer) {
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
  vkCmdFillBuffer(secondaryCommandBuffer, buffer, 0, 64, 32);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(secondaryCommandBuffer));

  vkCmdExecuteCommands(commandBuffer, 1, &secondaryCommandBuffer);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;
  DeviceMemoryTest::mapMemory(0, 64, &mappedMemory);
  for (int dataIndex = 0; dataIndex < 16; dataIndex++) {
    ASSERT_EQ(32u, static_cast<uint32_t *>(mappedMemory)[dataIndex]);
  }
  DeviceMemoryTest::unmapMemory();
  vkFreeCommandBuffers(device, commandPool, 1, &secondaryCommandBuffer);
}

