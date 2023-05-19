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

#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define TSAN_BUILD
#endif
#elif defined(__SANITIZE_THREAD__)
#define TSAN_BUILD
#endif

class SimultaneousUse : public uvk::PipelineTest,
                        public uvk::BufferTest,
                        public uvk::DeviceMemoryTest {
 public:
  SimultaneousUse()
      : BufferTest(
            bufferElements * sizeof(uint32_t),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            true),
        DeviceMemoryTest(true) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());
    vkGetDeviceQueue(device, 0, 0, &queue);

    RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

    vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer2);

    bufferBytes = BufferTest::bufferMemoryRequirements.size;
    DeviceMemoryTest::memorySize = bufferBytes * 2;
    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    vkBindBufferMemory(device, buffer, memory, 0);
    vkBindBufferMemory(device, buffer2, memory, bufferBytes);

    std::vector<uint32_t> data(bufferElements, 42);

    void *mappedMemory = nullptr;
    DeviceMemoryTest::mapMemory(0, bufferBytes, &mappedMemory);
    std::memcpy(mappedMemory, data.data(), bufferElements * sizeof(uint32_t));
    DeviceMemoryTest::unmapMemory();

    submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
  }

  virtual void TearDown() override {
    if (buffer2) {
      vkDestroyBuffer(device, buffer2, nullptr);
    }
    DeviceMemoryTest::TearDown();
    BufferTest::TearDown();
    PipelineTest::TearDown();
  }

  static const uint32_t bufferElements = 128;
  uint32_t bufferBytes;
  VkBuffer buffer2;
  VkBufferCopy copy;
  VkQueue queue;
  VkSubmitInfo submitInfo;
};

// This is a smoke test to check command buffers don't break in the event of
// irresponsible (but legal) API useage. It may cause a data race so the test
// is disabled for TSAN builds, but this is inconsequential to the test itself.
#ifdef TSAN_BUILD
TEST_F(SimultaneousUse, DISABLED_CmdCopyBuffer) {
#else
TEST_F(SimultaneousUse, CmdCopyBuffer) {
#endif
  // this test potentially causes a data race as both simultaneously submitted
  // copy commands will be attempting to copy buffer into buffer2
  VkBufferCopy copy;
  copy.size = bufferElements * sizeof(uint32_t);
  copy.dstOffset = 0;
  copy.srcOffset = 0;

  commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));
  ASSERT_EQ_RESULT(
      VK_SUCCESS, vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

  vkCmdCopyBuffer(commandBuffer, buffer, buffer2, 1, &copy);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(bufferBytes, bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferElements; i++) {
    ASSERT_EQ(42, static_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(SimultaneousUse, CmdDispatch) {
  commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));
  vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

// This is a smoke test to check command buffers don't break in the event of
// irresponsible (but legal) API useage. It may cause a data race so the test
// is disabled for TSAN builds, but this is inconsequential to the test itself.
#ifdef TSAN_BUILD
TEST_F(SimultaneousUse, DISABLED_CmdFillBuffer) {
#else
TEST_F(SimultaneousUse, CmdFillBuffer) {
#endif
  // this test potentially causes a data race (two on host due to the way the
  // fill buffer command is implemented) because both simultaneously submitted
  // fill buffer commands are attempting to fill the same buffer with 42s
  commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));
  ASSERT_EQ_RESULT(
      VK_SUCCESS, vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

  vkCmdFillBuffer(commandBuffer, buffer, 0, 64, 42);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;
  DeviceMemoryTest::mapMemory(0, bufferBytes, &mappedMemory);
  for (int dataIndex = 0; dataIndex < 16; dataIndex++) {
    ASSERT_EQ(42u, static_cast<uint32_t *>(mappedMemory)[dataIndex]);
  }
  DeviceMemoryTest::unmapMemory();
}

TEST_F(SimultaneousUse, CmdPipelineBarrier) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkBeginCommandBuffer(commandBuffer, &beginInfo));

  vkCmdFillBuffer(commandBuffer, buffer2, 0, VK_WHOLE_SIZE, 24);

  VkBufferMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.buffer = buffer2;
  memBarrier.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                       &memBarrier, 0, nullptr);
  vkCmdFillBuffer(commandBuffer, buffer2, 0, VK_WHOLE_SIZE, 42);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

// This is a smoke test to check command buffers don't break in the event of
// irresponsible (but legal) API useage. It may cause a data race so the test
// is disabled for TSAN builds, but this is inconsequential to the test itself.
#ifdef TSAN_BUILD
TEST_F(SimultaneousUse, DISABLED_CmdUpdateBuffer) {
#else
TEST_F(SimultaneousUse, CmdUpdateBuffer) {
#endif
  // this test potentially causes a data race as both simultaneously submitted
  // update buffer commands will be attemptig to copy the contents of data into
  // the buffer
  commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));
  ASSERT_EQ_RESULT(
      VK_SUCCESS, vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

  std::vector<uint32_t> data(bufferElements, 42);
  vkCmdUpdateBuffer(commandBuffer, buffer, 0, bufferElements * sizeof(uint32_t),
                    data.data());
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, bufferBytes, &mappedMemory);

  for (uint32_t dataIndex = 0; dataIndex < bufferElements; dataIndex++) {
    ASSERT_EQ(42u, static_cast<uint32_t *>(mappedMemory)[dataIndex]);
  }

  DeviceMemoryTest::unmapMemory();
}

// This is a smoke test to check command buffers don't break in the event of
// irresponsible (but legal) API usage. It may cause a data race so the test
// is disabled for TSAN builds, but this is inconsequential to the test itself.
#ifdef TSAN_BUILD
TEST_F(SimultaneousUse, DISABLED_SecondaryCommandBuffer) {
#else
TEST_F(SimultaneousUse, SecondaryCommandBuffer) {
#endif
  // this test potentially causes a data race as the secondary command buffer
  // has a fill buffer command recorded into it so it creates the same
  // conditions as SimultaneousUse.CmdFilleBuffer
  VkCommandBufferAllocateInfo allocInf = {};
  allocInf.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInf.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  allocInf.commandPool = commandPool;
  allocInf.commandBufferCount = 1;

  VkCommandBuffer secondaryCommandBuffer;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateCommandBuffers(
                                   device, &allocInf, &secondaryCommandBuffer));

  allocInf = {};
  allocInf.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInf.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInf.commandPool = commandPool;
  allocInf.commandBufferCount = 1;

  VkCommandBuffer commandBuffer2;
  ASSERT_EQ_RESULT(
      VK_SUCCESS, vkAllocateCommandBuffers(device, &allocInf, &commandBuffer2));

  VkCommandBufferInheritanceInfo inheritInfo = {};
  inheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
  inheritInfo.framebuffer = VK_NULL_HANDLE;
  inheritInfo.occlusionQueryEnable = VK_FALSE;

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = &inheritInfo;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkBeginCommandBuffer(secondaryCommandBuffer, &beginInfo));

  vkCmdFillBuffer(secondaryCommandBuffer, buffer, 0, bufferElements, 42);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(secondaryCommandBuffer));

  vkCmdExecuteCommands(commandBuffer, 1, &secondaryCommandBuffer);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  // prepare second command buffer
  ASSERT_EQ_RESULT(VK_SUCCESS, vkBeginCommandBuffer(commandBuffer2,
                                                    &commandBufferBeginInfo));

  vkCmdExecuteCommands(commandBuffer2, 1, &secondaryCommandBuffer);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer2));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  submitInfo.pCommandBuffers = &commandBuffer2;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

#undef TSAN_BUILD
