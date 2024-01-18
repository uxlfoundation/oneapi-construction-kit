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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCmdPipelineBarrier

#define WORK_ITEMS 16
class CmdPipelineBarrier : public uvk::PipelineTest,
                           public uvk::DescriptorPoolTest,
                           public uvk::DescriptorSetLayoutTest,
                           public uvk::DeviceMemoryTest,
                           public uvk::BufferTest {
 public:
  CmdPipelineBarrier()
      : PipelineTest(uvk::Shader::mov),
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

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
  }

  virtual void TearDown() override {
    vkDestroyBuffer(device, buffer2, nullptr);

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
};

TEST_F(CmdPipelineBarrier, DefaultSrcTransferDstCompute) {
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdFillBuffer(commandBuffer, buffer2, 0, VK_WHOLE_SIZE, 24);

  VkBufferMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.buffer = buffer2;
  memBarrier.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                       &memBarrier, 0, nullptr);
  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  // if the dispatch was executed after the fill buffer this will be 42s
  // instead of 24s
  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(42u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdPipelineBarrier, DefaultSrcComputeDstTransfer) {
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  VkBufferMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.buffer = buffer2;
  memBarrier.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                       &memBarrier, 0, nullptr);
  vkCmdFillBuffer(commandBuffer, buffer2, 0, VK_WHOLE_SIZE, 24);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  // if the fill buffer was executed after the dispatch this will be 24s
  // instead of 42s
  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(24u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdPipelineBarrier, DefaultSrcComputeDstCompute) {
  PipelineTest::shader = uvk::Shader::chain;
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  VkBufferMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarrier.dstAccessMask =
      VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memBarrier.srcQueueFamilyIndex = queueFamilyIndex;
  memBarrier.dstQueueFamilyIndex = queueFamilyIndex;
  memBarrier.buffer = buffer2;
  memBarrier.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                       &memBarrier, 0, nullptr);
  vkCmdDispatch(commandBuffer, 2, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  // check the second dispatch was executed after the first dispatch
  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(42u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdPipelineBarrier, DefaultSrcTransferDstTransfer) {
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

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

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  // check that the second fill buffer command was executed after the first
  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(42u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdPipelineBarrier, DefaultSecondaryCommandBuffer) {
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  VkCommandBuffer sCommandBuffer;

  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
  commandBufferAllocateInfo.sType =
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandBufferCount = 1;
  commandBufferAllocateInfo.commandPool = commandPool;
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkAllocateCommandBuffers(device, &commandBufferAllocateInfo,
                                            &sCommandBuffer));

  VkCommandBufferInheritanceInfo inheritInfo = {};
  inheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

  VkCommandBufferBeginInfo cbBeginInfo = {};
  cbBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cbBeginInfo.pInheritanceInfo = &inheritInfo;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkBeginCommandBuffer(sCommandBuffer, &cbBeginInfo));

  vkCmdBindDescriptorSets(sCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(sCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdFillBuffer(sCommandBuffer, buffer2, 0, VK_WHOLE_SIZE, 24);

  VkBufferMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarrier.buffer = buffer2;
  memBarrier.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(sCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                       &memBarrier, 0, nullptr);
  vkCmdDispatch(sCommandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(sCommandBuffer));

  vkCmdExecuteCommands(commandBuffer, 1, &sCommandBuffer);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  // if the dispatch was executed after the fill buffer this will be 42s
  // instead of 24s
  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(42u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdPipelineBarrier, AcrossPipelines) {
  PipelineTest::shader = uvk::Shader::delay;
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  const uvk::ShaderCode shaderCode = uvk::getShader(uvk::Shader::write_back);

  VkShaderModuleCreateInfo shaderCreateInfo = {};
  shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderCreateInfo.pCode = reinterpret_cast<const uint32_t *>(shaderCode.code);
  shaderCreateInfo.codeSize = shaderCode.size;

  VkShaderModule shaderModule;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateShaderModule(device, &shaderCreateInfo,
                                                    nullptr, &shaderModule));

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
  shaderStageCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.module = shaderModule;
  shaderStageCreateInfo.pName = "main";
  shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageCreateInfo.pSpecializationInfo = nullptr;

  VkPipeline backPipeline;
  VkComputePipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = pipelineLayout;
  pipelineCreateInfo.stage = shaderStageCreateInfo;
  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                               nullptr, &backPipeline));

  vkDestroyShaderModule(device, shaderModule, nullptr);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  VkBufferMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarrier.dstAccessMask =
      VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
  memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memBarrier.srcQueueFamilyIndex = queueFamilyIndex;
  memBarrier.dstQueueFamilyIndex = queueFamilyIndex;
  memBarrier.buffer = buffer2;
  memBarrier.size = VK_WHOLE_SIZE;

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    backPipeline);

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
                       &memBarrier, 0, nullptr);

  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, bufferBytes, &mappedMemory);

  // check the second dispatch was executed after the first dispatch
  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(42u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
  vkDestroyPipeline(device, backPipeline, nullptr);
}

TEST_F(CmdPipelineBarrier, FillToCopy) {
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  vkCmdFillBuffer(commandBuffer, buffer, 0, VK_WHOLE_SIZE, 24);
  vkCmdFillBuffer(commandBuffer, buffer2, 0, VK_WHOLE_SIZE, 42);
  VkMemoryBarrier memBarrier = {};
  memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memBarrier, 0,
                       nullptr, 0, nullptr);

  VkBufferCopy bufferCopy = {};
  bufferCopy.size = bufferBytes;
  vkCmdCopyBuffer(commandBuffer, buffer, buffer2, 1, &bufferCopy);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(alignedDeviceSize(bufferMemoryRequirements),
                              bufferBytes, &mappedMemory);

  // check that the copy was executed after the fills
  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(24u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdPipelineBarrier, Stress) {
  PipelineTest::shader = uvk::Shader::turns;
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 1, 1, WORK_ITEMS);

  const size_t iterations = 20;
  for (size_t i = 1; i < iterations; i++) {
    VkMemoryBarrier memBarrier = {};
    memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memBarrier.srcAccessMask =
        VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
    memBarrier.dstAccessMask =
        VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memBarrier, 0,
                         nullptr, 0, nullptr);

    vkCmdDispatch(commandBuffer, 1 + (i % 2), i + 1, WORK_ITEMS);
  }
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  submitInfo.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, bufferBytes, &mappedMemory);

  // Check that all dispatches have been executed
  for (uint32_t i = 0; i < bufferBytes / sizeof(int32_t); i++) {
    ASSERT_EQ(210u + 42u, reinterpret_cast<uint32_t *>(mappedMemory)[i]);
  }

  DeviceMemoryTest::unmapMemory();
}
