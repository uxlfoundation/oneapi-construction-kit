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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkWaitForFences

class WaitForFences : public uvk::RecordCommandBufferTest,
                      uvk::DeviceMemoryTest,
                      uvk::BufferTest {
 public:
  WaitForFences()
      : uvk::DeviceMemoryTest(true),
        uvk::BufferTest(32, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true),
        submitInfo() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(RecordCommandBufferTest::SetUp());

    RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

    DeviceMemoryTest::memorySize =
        BufferTest::bufferMemoryRequirements.size * 2;

    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer2);

    vkBindBufferMemory(device, buffer, memory, 0);
    vkBindBufferMemory(device, buffer2, memory,
                       BufferTest::bufferMemoryRequirements.size);

    // some meaningless work to make the command buffer actually do something
    vkCmdFillBuffer(commandBuffer, buffer, 0, VK_WHOLE_SIZE, 42);

    vkEndCommandBuffer(commandBuffer);

    CreateAndRecordCommandBuffer(&commandBuffer2);

    vkCmdFillBuffer(commandBuffer2, buffer2, 0, VK_WHOLE_SIZE, 42);

    vkEndCommandBuffer(commandBuffer2);

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    vkCreateFence(device, &fenceCreateInfo, nullptr, &fence1);
    vkCreateFence(device, &fenceCreateInfo, nullptr, &fence2);

    vkGetDeviceQueue(device, 0, 0, &queue);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
  }

  virtual void TearDown() override {
    vkDestroyFence(device, fence1, nullptr);
    vkDestroyFence(device, fence2, nullptr);

    vkDestroyBuffer(device, buffer2, nullptr);

    BufferTest::TearDown();
    DeviceMemoryTest::TearDown();
    RecordCommandBufferTest::TearDown();
  }

  VkBuffer buffer2;
  VkDeviceMemory memory2;
  VkQueue queue;
  VkFence fence1, fence2;
  VkCommandBuffer commandBuffer2;
  VkSubmitInfo submitInfo;
  // the timeout is a minute in nanoseconds
  const uint64_t timeout = 10000000000;
};

TEST_F(WaitForFences, Default) {
  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence1));

  submitInfo.pCommandBuffers = &commandBuffer2;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence2));

  std::vector<VkFence> waitFences = {fence1, fence2};

  ASSERT_EQ_RESULT(VK_SUCCESS, vkWaitForFences(device, 2, waitFences.data(),
                                               VK_FALSE, timeout));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

TEST_F(WaitForFences, DefaultWaitAll) {
  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence1));

  submitInfo.pCommandBuffers = &commandBuffer2;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence2));

  std::vector<VkFence> waitFences = {fence1, fence2};

  ASSERT_EQ_RESULT(VK_SUCCESS, vkWaitForFences(device, 2, waitFences.data(),
                                               VK_TRUE, timeout));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

TEST_F(WaitForFences, DefaultTimeout) {
  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence1));

  std::vector<VkFence> waitFences = {fence1, fence2};

  ASSERT_EQ_RESULT(VK_TIMEOUT,
                   vkWaitForFences(device, 2, waitFences.data(), VK_TRUE, 1));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

TEST_F(WaitForFences, MaxTimeout) {
  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence1));

  submitInfo.pCommandBuffers = &commandBuffer2;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence2));

  std::vector<VkFence> waitFences = {fence1, fence2};

  ASSERT_EQ_RESULT(VK_SUCCESS, vkWaitForFences(device, 2, waitFences.data(),
                                               VK_TRUE, UINT64_MAX));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

// second threads function for multithreaded tests
static void thread2fn(WaitForFences* pTestFixture, const uint64_t timeout) {
  VkFence waitFences[2];
  waitFences[0] = pTestFixture->fence1;
  waitFences[1] = pTestFixture->fence2;
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkWaitForFences(pTestFixture->device, 2, waitFences, VK_TRUE,
                                   timeout));
}

TEST_F(WaitForFences, MultithreadedWait) {
  // submit 1st command buffer
  submitInfo.pCommandBuffers = &commandBuffer;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence1));

  // and the second
  submitInfo.pCommandBuffers = &commandBuffer2;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence2));

  // start up child thread
  std::thread thread2(thread2fn, this, timeout);

  // wait for both threads to finish
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkWaitForFences(device, 1, &fence1, VK_FALSE, timeout));
  thread2.join();

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

TEST_F(WaitForFences, MultithreadedWaitAll) {
  // submit 1st command buffer
  submitInfo.pCommandBuffers = &commandBuffer;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence1));

  // and the second
  submitInfo.pCommandBuffers = &commandBuffer2;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence2));

  // start up child thread
  std::thread thread2(thread2fn, this, timeout);

  // wait for this thread and thread2 to finish
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkWaitForFences(device, 1, &fence1, VK_TRUE, timeout));
  thread2.join();

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

TEST_F(WaitForFences, MultithreadedWaitAlreadyFinished) {
  // submit command buffers
  submitInfo.pCommandBuffers = &commandBuffer;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence1));
  submitInfo.pCommandBuffers = &commandBuffer2;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueSubmit(queue, 1, &submitInfo, fence2));

  // wait for both submissions to finish
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  // start up child thread
  std::thread thread2(thread2fn, this, timeout);

  // wait for all fences (should be instantaneous as already complete)
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkWaitForFences(device, 1, &fence1, VK_TRUE, timeout));
  thread2.join();

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

