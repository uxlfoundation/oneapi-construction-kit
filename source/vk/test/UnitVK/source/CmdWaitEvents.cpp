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

// https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#vkCmdWaitEvents

#define WORK_ITEMS 16

class CmdWaitEvents : public uvk::PipelineTest,
                      public uvk::DescriptorPoolTest,
                      public uvk::DescriptorSetLayoutTest,
                      public uvk::DeviceMemoryTest,
                      public uvk::BufferTest {
 public:
  CmdWaitEvents()
      : PipelineTest(uvk::Shader::mov),
        DescriptorPoolTest(true),
        DescriptorSetLayoutTest(true),
        DeviceMemoryTest(true),
        BufferTest(WORK_ITEMS * sizeof(int32_t),
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   true),
        bufferBytes(WORK_ITEMS * sizeof(int32_t)),
        submitInfo(),
        queue(VK_NULL_HANDLE),
        event(VK_NULL_HANDLE) {}

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

    VkEventCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreateEvent(device, &createInfo, nullptr, &event));

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  }

  virtual void TearDown() override {
    if (buffer2) {
      vkDestroyBuffer(device, buffer2, nullptr);
    }
    if (event) {
      vkDestroyEvent(device, event, nullptr);
    }

    BufferTest::TearDown();
    DeviceMemoryTest::TearDown();
    DescriptorSetLayoutTest::TearDown();
    DescriptorPoolTest::TearDown();
    PipelineTest::TearDown();
  }

  VkBuffer buffer2;
  uint32_t bufferBytes;
  VkDescriptorSet descriptorSet;
  VkSubmitInfo submitInfo;
  VkQueue queue;
  VkEvent event;
};

class CmdWaitEventsCommandBuffers : public CmdWaitEvents {
 public:
  CmdWaitEventsCommandBuffers() : CmdWaitEvents() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(CmdWaitEvents::SetUp());
    RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

    CreateAndRecordCommandBuffer(&commandBuffer2);
  }

  virtual void TearDown() override { CmdWaitEvents::TearDown(); }

  VkCommandBuffer commandBuffer2;
};

TEST_F(CmdWaitEventsCommandBuffers, MultipleCommandBuffers) {
  vkCmdFillBuffer(commandBuffer2, buffer, 0, VK_WHOLE_SIZE, 24);

  vkCmdSetEvent(commandBuffer2, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer2));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  VkBufferMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.buffer = buffer;
  memBarrier.size = VK_WHOLE_SIZE;

  vkCmdWaitEvents(commandBuffer, 1, &event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 1,
                  &memBarrier, 0, nullptr);

  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkCommandBuffer commandBuffers[] = {commandBuffer2, commandBuffer};
  submitInfo.commandBufferCount = 2;
  submitInfo.pCommandBuffers = commandBuffers;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(24u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdWaitEvents, SingleCommandBuffer) {
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());
  vkCmdFillBuffer(commandBuffer, buffer, 0, VK_WHOLE_SIZE, 24);

  vkCmdSetEvent(commandBuffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  VkBufferMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.buffer = buffer;
  memBarrier.size = VK_WHOLE_SIZE;

  vkCmdWaitEvents(commandBuffer, 1, &event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 1,
                  &memBarrier, 0, nullptr);

  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferBytes / 4; i++) {
    ASSERT_EQ(24u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdWaitEventsCommandBuffers, MultipleSubmissions) {
  vkCmdFillBuffer(commandBuffer2, buffer, 0, VK_WHOLE_SIZE, 24);

  vkCmdSetEvent(commandBuffer2, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer2));

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer2;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  VkBufferMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.buffer = buffer;
  memBarrier.size = VK_WHOLE_SIZE;

  vkCmdWaitEvents(commandBuffer, 1, &event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 1,
                  &memBarrier, 0, nullptr);

  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(24u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdWaitEvents, MultipleWaits) {
  PipelineTest::shader = uvk::Shader::chain;
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());
  vkCmdFillBuffer(commandBuffer, buffer, 0, VK_WHOLE_SIZE, 24);

  vkCmdSetEvent(commandBuffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  VkBufferMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.buffer = buffer;
  memBarrier.size = VK_WHOLE_SIZE;

  vkCmdWaitEvents(commandBuffer, 1, &event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 1,
                  &memBarrier, 0, nullptr);

  vkCmdResetEvent(commandBuffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  vkCmdSetEvent(commandBuffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memBarrier.dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  memBarrier.buffer = buffer2;

  vkCmdWaitEvents(commandBuffer, 1, &event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 1,
                  &memBarrier, 0, nullptr);

  vkCmdDispatch(commandBuffer, 2, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(24u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }
  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdWaitEventsCommandBuffers, HostSet) {
  vkCmdFillBuffer(commandBuffer2, buffer, 0, VK_WHOLE_SIZE, 24);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer2));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  vkCmdWaitEvents(commandBuffer, 1, &event, VK_PIPELINE_STAGE_HOST_BIT,
                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 0,
                  nullptr);

  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer2;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkSetEvent(device, event));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  for (uint32_t i = 0; i < bufferBytes / sizeof(uint32_t); i++) {
    ASSERT_EQ(24u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}
