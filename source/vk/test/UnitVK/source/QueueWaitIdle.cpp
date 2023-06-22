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

#include <thread>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkQueueWaitIdle

class QueueWaitIdle : public uvk::RecordCommandBufferTest,
                      public uvk::BufferTest,
                      public uvk::DeviceMemoryTest {
 public:
  QueueWaitIdle()
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

TEST_F(QueueWaitIdle, Default) {
  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

TEST_F(QueueWaitIdle, MultithreadedSameQueue) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  auto thread2func = [](VkQueue queue) {
    ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
  };

  // Start second thread
  std::thread thread2(thread2func, queue);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
  thread2.join();
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
//
// VK_ERROR_DEVICE_LOST
// Is a possible return from this function, but is untestable
// as the conditions it returns under cannot be safely replicated
