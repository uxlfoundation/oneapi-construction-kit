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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkQueueSubmit

class QueueSubmit : public uvk::RecordCommandBufferTest,
                    public uvk::BufferTest,
                    public uvk::DeviceMemoryTest {
 public:
  QueueSubmit()
      : BufferTest(16 * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   true),
        DeviceMemoryTest(true),
        bufferSize(16 * sizeof(uint32_t)),
        queue(VK_NULL_HANDLE),
        submitInfo() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(RecordCommandBufferTest::SetUp());
    vkGetDeviceQueue(device, 0, 0, &queue);

    RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

    DeviceMemoryTest::memorySize = BufferTest::bufferMemoryRequirements.size;
    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    vkBindBufferMemory(device, buffer, memory, 0);

    vkCmdFillBuffer(commandBuffer, buffer, 0, bufferSize, 42);

    vkEndCommandBuffer(commandBuffer);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
  }

  virtual void TearDown() override {
    BufferTest::TearDown();
    DeviceMemoryTest::TearDown();
    RecordCommandBufferTest::TearDown();
  }

  uint32_t bufferSize;
  VkQueue queue;
  VkSubmitInfo submitInfo;
};

TEST_F(QueueSubmit, Default) {
  // TODO: add some commands to the command buffer to make this test actually
  // test stuff
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

TEST_F(QueueSubmit, DefaultSignalSemaphore) {
  VkSemaphore semaphore;

  VkSemaphoreCreateInfo semaphoreCreateInfo = {};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateSemaphore(device, &semaphoreCreateInfo,
                                                 nullptr, &semaphore));

  submitInfo.pSignalSemaphores = &semaphore;
  submitInfo.signalSemaphoreCount = 1;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  vkDestroySemaphore(device, semaphore, nullptr);
}

TEST_F(QueueSubmit, DefaultWaitSemaphore) {
  VkSemaphore semaphore;

  VkSemaphoreCreateInfo semaphoreCreateInfo = {};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateSemaphore(device, &semaphoreCreateInfo,
                                                 nullptr, &semaphore));

  submitInfo.pSignalSemaphores = &semaphore;
  submitInfo.signalSemaphoreCount = 1;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

  submitInfo.signalSemaphoreCount = 0;
  submitInfo.pSignalSemaphores = nullptr;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &semaphore;
  submitInfo.pWaitDstStageMask = &waitStage;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  vkDestroySemaphore(device, semaphore, nullptr);
}

TEST_F(QueueSubmit, DefaultOneTimeSubmit) {
  // Reset existing command buffer and begin with new flags
  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetCommandBuffer(commandBuffer, 0));

  commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  // prepare command buffer
  ASSERT_EQ_RESULT(
      VK_SUCCESS, vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
  vkCmdFillBuffer(commandBuffer, buffer, 0, bufferSize, 42);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  // submit for first time
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  // wait for work to finish
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  // reset and do it all again
  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetCommandBuffer(commandBuffer, 0));

  // the value that the first fill buffer should be overwritten with
  const uint32_t secondSubmitFillValue = 24;

  ASSERT_EQ_RESULT(
      VK_SUCCESS, vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
  vkCmdFillBuffer(commandBuffer, buffer, 0, bufferSize, secondSubmitFillValue);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  // if the second submit succeeded the 42s will have been overwritten with 24s
  for (uint32_t mem_index = 0; mem_index < bufferSize / sizeof(uint32_t);
       mem_index++) {
    ASSERT_EQ(static_cast<uint32_t *>(mappedMemory)[mem_index],
              secondSubmitFillValue);
  }

  DeviceMemoryTest::unmapMemory();
}

// COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT cannot be negatively tested as it fails
// via VK_ABORT

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
//
// VK_ERROR_DEVICE_LOST
// Is a possible return from this function, but is untestable
// as the conditions it returns under cannot be safely replicated
