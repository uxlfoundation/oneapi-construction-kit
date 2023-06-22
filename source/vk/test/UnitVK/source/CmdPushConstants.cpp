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

// https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#vkCmdPushConstants

class CmdPushConstants : public uvk::PipelineTest,
                         public uvk::DeviceMemoryTest,
                         public uvk::BufferTest,
                         public uvk::DescriptorPoolTest {
 public:
  CmdPushConstants()
      : PipelineTest(uvk::Shader::push_constant),
        DeviceMemoryTest(true),
        BufferTest(sizeof(uint32_t),
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   true),
        DescriptorPoolTest(true),
        pushConstant(42),
        submitInfo() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

    memorySize = BufferTest::bufferMemoryRequirements.size;

    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer, memory, 0));

    RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
         NULL}};

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInf = {};
    descriptorSetLayoutCreateInf.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInf.bindingCount = bindings.size();
    descriptorSetLayoutCreateInf.pBindings = bindings.data();

    ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateDescriptorSetLayout(
                                     device, &descriptorSetLayoutCreateInf,
                                     nullptr, &descriptorSetLayout));

    VkDescriptorSetAllocateInfo dSetAllocInf = {};
    dSetAllocInf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dSetAllocInf.descriptorSetCount = 1;
    dSetAllocInf.pSetLayouts = &descriptorSetLayout;
    dSetAllocInf.descriptorPool = descriptorPool;

    ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateDescriptorSets(device, &dSetAllocInf,
                                                          &descriptorSet));

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

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(pushConstant);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayout layouts[2] = {descriptorSetLayout,
                                        descriptorSetLayout};

    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutCreateInfo.setLayoutCount = 2;
    pipelineLayoutCreateInfo.pSetLayouts = layouts;

    RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

    vkGetDeviceQueue(device, 0, 0, &queue);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
  }

  virtual void TearDown() override {
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    DescriptorPoolTest::TearDown();
    BufferTest::TearDown();
    DeviceMemoryTest::TearDown();

    PipelineTest::TearDown();
  }

  uint32_t pushConstant;
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSet descriptorSet;
  VkSubmitInfo submitInfo;
  VkQueue queue;
};

TEST_F(CmdPushConstants, Default) {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                     0, sizeof(pushConstant), &pushConstant);
  vkCmdDispatch(commandBuffer, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  ASSERT_EQ(pushConstant, *reinterpret_cast<uint32_t *>(mappedMemory));

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdPushConstants, MultipleCommandBuffers) {
  // Create first command buffer.
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                     0, sizeof(pushConstant), &pushConstant);
  vkCmdDispatch(commandBuffer, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  // Create second command buffer.
  VkCommandBuffer commandBuffer2;
  auto pushConstant2 = pushConstant * 2;
  CreateAndRecordCommandBuffer(&commandBuffer2);
  vkCmdBindPipeline(commandBuffer2, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer2, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer2, pipelineLayout,
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstant),
                     &pushConstant2);
  vkCmdDispatch(commandBuffer2, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer2));

  void *mappedMemory;

  // Run first command buffer.
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  ASSERT_EQ(pushConstant, *reinterpret_cast<uint32_t *>(mappedMemory));

  DeviceMemoryTest::unmapMemory();

  // Run second command buffer.
  auto secondSubmitInfo = submitInfo;
  secondSubmitInfo.pCommandBuffers = &commandBuffer2;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &secondSubmitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  ASSERT_EQ(pushConstant2, *reinterpret_cast<uint32_t *>(mappedMemory));

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdPushConstants, DefaultPushConstantsBeforeBindings) {
  vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                     0, sizeof(pushConstant), &pushConstant);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdDispatch(commandBuffer, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  ASSERT_EQ(pushConstant, *reinterpret_cast<uint32_t *>(mappedMemory));

  DeviceMemoryTest::unmapMemory();
}

TEST_F(CmdPushConstants, DefaultSecondaryCommandBuffer) {
  VkCommandBuffer secondaryCommandBuffer;

  VkCommandBufferAllocateInfo sCommandBufferAllocInf = {};
  sCommandBufferAllocInf.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  sCommandBufferAllocInf.commandBufferCount = 1;
  sCommandBufferAllocInf.commandPool = commandPool;
  sCommandBufferAllocInf.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkAllocateCommandBuffers(device, &sCommandBufferAllocInf,
                                            &secondaryCommandBuffer));

  VkCommandBufferInheritanceInfo inheritanceInfo = {};
  inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = &inheritanceInfo;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkBeginCommandBuffer(secondaryCommandBuffer, &beginInfo));
  vkCmdPushConstants(secondaryCommandBuffer, pipelineLayout,
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstant),
                     &pushConstant);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(secondaryCommandBuffer));

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdExecuteCommands(commandBuffer, 1, &secondaryCommandBuffer);
  vkCmdDispatch(commandBuffer, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  ASSERT_EQ(pushConstant, *reinterpret_cast<uint32_t *>(mappedMemory));

  DeviceMemoryTest::unmapMemory();

  vkFreeCommandBuffers(device, commandPool, 1, &secondaryCommandBuffer);
}

TEST_F(CmdPushConstants, DefaultBindUnusedDescriptorSet) {
  VkDescriptorSet descriptorSetB;

  VkDescriptorSetAllocateInfo dSetAllocInfo = {};
  dSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dSetAllocInfo.descriptorPool = descriptorPool;
  dSetAllocInfo.descriptorSetCount = 1;
  dSetAllocInfo.pSetLayouts = &descriptorSetLayout;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateDescriptorSets(device, &dSetAllocInfo,
                                                        &descriptorSetB));

  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.offset = 0;
  bufferInfo.range = VK_WHOLE_SIZE;
  bufferInfo.buffer = buffer;

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write.dstBinding = 0;
  write.dstArrayElement = 0;
  write.dstSet = descriptorSetB;
  write.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  VkDescriptorSet sets[2] = {descriptorSet, descriptorSetB};

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 2, sets, 0, nullptr);
  vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                     0, sizeof(pushConstant), &pushConstant);
  vkCmdDispatch(commandBuffer, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  ASSERT_EQ(pushConstant, *reinterpret_cast<uint32_t *>(mappedMemory));

  DeviceMemoryTest::unmapMemory();
}
