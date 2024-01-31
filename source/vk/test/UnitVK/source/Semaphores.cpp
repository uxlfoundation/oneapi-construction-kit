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

#define WORK_ITEMS 16

// https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-semaphores

class Semaphores : public uvk::PipelineTest,
                   public uvk::DescriptorPoolTest,
                   public uvk::DescriptorSetLayoutTest,
                   public uvk::DeviceMemoryTest,
                   public uvk::BufferTest {
 public:
  Semaphores()
      : PipelineTest(uvk::Shader::chain),
        DescriptorPoolTest(true),
        DescriptorSetLayoutTest(true),
        DeviceMemoryTest(true),
        BufferTest(sizeof(int32_t) * WORK_ITEMS,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   true),
        bufferBytes(sizeof(int32_t) * WORK_ITEMS),
        submitInfo(),
        queue(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    descriptorSetLayoutBindings = {
        {
            0,                                  // binding
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // type
            1,                                  // count
            VK_SHADER_STAGE_COMPUTE_BIT,        // stage flags
            nullptr                             // immutable samplers
        },
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
         nullptr}};

    RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    const VkDescriptorPoolSize poolSize = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                           2};

    DescriptorPoolTest::poolSizes.push_back(poolSize);
    RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

    VkDescriptorSetAllocateInfo dSetAllocInfo = {};
    dSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dSetAllocInfo.descriptorPool = descriptorPool;
    dSetAllocInfo.descriptorSetCount = 1;
    dSetAllocInfo.pSetLayouts = &descriptorSetLayout;

    ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateDescriptorSets(
                                     device, &dSetAllocInfo, &descriptorSet));

    RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

    ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateBuffer(device, &bufferCreateInfo,
                                                nullptr, &buffer2));

    const VkDeviceSize alignedBufferSize =
        alignedDeviceSize(bufferMemoryRequirements);
    DeviceMemoryTest::memorySize = alignedBufferSize * 2;
    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer, memory, 0));
    ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer2, memory,
                                                    alignedBufferSize));

    void *mappedMemory;

    uint32_t data = 42;

    DeviceMemoryTest::mapMemory(0, bufferBytes, &mappedMemory);

    int32_t *devicePtr = static_cast<int32_t *>(mappedMemory);
    for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
      memcpy(devicePtr, &data, sizeof(uint32_t));
      devicePtr++;
    }

    DeviceMemoryTest::unmapMemory();

    std::vector<VkWriteDescriptorSet> writes;

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.dstArrayElement = 0;
    write.dstBinding = 0;
    write.dstSet = descriptorSet;
    write.pBufferInfo = &bufferInfo;

    writes.push_back(write);

    VkDescriptorBufferInfo buffer2Info = {};
    buffer2Info.buffer = buffer2;
    buffer2Info.offset = 0;
    buffer2Info.range = VK_WHOLE_SIZE;

    write.pBufferInfo = &buffer2Info;
    write.dstBinding = 1;

    writes.push_back(write);

    vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);

    vkGetDeviceQueue(device, 0, 0, &queue);

    RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

    CreateAndRecordCommandBuffer(&commandBuffer2);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateSemaphore(device, &semaphoreCreateInfo,
                                                   nullptr, &semaphore));
  }

  virtual void TearDown() override {
    if (buffer2) {
      vkDestroyBuffer(device, buffer2, nullptr);
    }
    if (semaphore) {
      vkDestroySemaphore(device, semaphore, nullptr);
    }

    BufferTest::TearDown();
    DeviceMemoryTest::TearDown();
    DescriptorSetLayoutTest::TearDown();
    DescriptorPoolTest::TearDown();
    PipelineTest::TearDown();
  }

  VkBuffer buffer2;
  uint32_t bufferBytes;
  VkSemaphore semaphore;
  VkDescriptorSet descriptorSet;
  VkCommandBuffer commandBuffer2;
  VkSubmitInfo submitInfo;
  VkQueue queue;
};

TEST_F(Semaphores, Basic) {
  vkCmdFillBuffer(commandBuffer2, buffer, 0, VK_WHOLE_SIZE, 24);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer2));

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer2;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &semaphore;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &semaphore;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.signalSemaphoreCount = 0;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(25u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(Semaphores, TwoSemaphores) {
  VkSemaphore semaphore2;
  VkSemaphoreCreateInfo semaphoreCreateInfo = {};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateSemaphore(device, &semaphoreCreateInfo,
                                                 nullptr, &semaphore2));

  vkCmdFillBuffer(commandBuffer2, buffer, 0, VK_WHOLE_SIZE, 24);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer2));

  VkSemaphore semaphores[] = {semaphore, semaphore2};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer2;
  submitInfo.signalSemaphoreCount = 2;
  submitInfo.pSignalSemaphores = semaphores;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  submitInfo.waitSemaphoreCount = 2;
  submitInfo.pWaitSemaphores = semaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(25u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
  vkDestroySemaphore(device, semaphore2, nullptr);
}

TEST_F(Semaphores, TwoCommandBuffers) {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandBufferCount = 2;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;

  VkCommandBuffer moreCommandBuffers[2];
  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateCommandBuffers(device, &allocInfo,
                                                        moreCommandBuffers));

  vkCmdFillBuffer(commandBuffer2, buffer, 0, VK_WHOLE_SIZE, 24);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer2));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  VkMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memBarrier,
                       0, nullptr, 0, nullptr);

  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkCommandBuffer commandBuffers[] = {commandBuffer2, commandBuffer};

  submitInfo.commandBufferCount = 2;
  submitInfo.pCommandBuffers = commandBuffers;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &semaphore;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  commandBufferBeginInfo = {};
  commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkBeginCommandBuffer(moreCommandBuffers[0],
                                                    &commandBufferBeginInfo));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkBeginCommandBuffer(moreCommandBuffers[1],
                                                    &commandBufferBeginInfo));

  vkCmdFillBuffer(moreCommandBuffers[1], buffer, 0, VK_WHOLE_SIZE, 42);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(moreCommandBuffers[1]));

  vkCmdBindDescriptorSets(moreCommandBuffers[0], VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(moreCommandBuffers[0], VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline);

  vkCmdDispatch(moreCommandBuffers[0], 2, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(moreCommandBuffers[0]));

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &semaphore;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.signalSemaphoreCount = 0;
  submitInfo.pCommandBuffers = moreCommandBuffers;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(42u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(24u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(Semaphores, ThreeSubmits) {
  vkCmdFillBuffer(commandBuffer2, buffer, 0, VK_WHOLE_SIZE, 24);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer2));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandBufferCount = 1;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;

  VkCommandBuffer commandBuffer3;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateCommandBuffers(device, &allocInfo,
                                                        &commandBuffer3));

  commandBufferBeginInfo = {};
  commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkBeginCommandBuffer(commandBuffer3,
                                                    &commandBufferBeginInfo));

  vkCmdBindDescriptorSets(commandBuffer3, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer3, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  vkCmdDispatch(commandBuffer3, 2, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer3));

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer2;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &semaphore;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &semaphore;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  submitInfo.pCommandBuffers = &commandBuffer3;
  submitInfo.signalSemaphoreCount = 0;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(24u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}
