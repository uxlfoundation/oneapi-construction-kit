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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCmdBindDescriptorSets

class CmdBindDescriptorSets : public uvk::RecordCommandBufferTest,
                              public uvk::DescriptorPoolTest,
                              public uvk::DeviceMemoryTest,
                              public uvk::PipelineLayoutTest {
 public:
  CmdBindDescriptorSets()
      : DescriptorPoolTest(true),
        DeviceMemoryTest(true),
        PipelineLayoutTest(true),
        bufferA(VK_NULL_HANDLE),
        bufferB(VK_NULL_HANDLE),
        descriptorSet(VK_NULL_HANDLE),
        submitInfo(),
        queueFamilyIndex(0),
        // completely arbitrary test value, the buffers never contain anything
        // and are just there so we can test binding a valid, updated descriptor
        // set
        bufferSize(16) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(RecordCommandBufferTest::SetUp());

    DescriptorPoolTest::poolSizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1}};

    RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

    DescriptorSetLayoutTest::descriptorSetLayoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, VK_SHADER_STAGE_COMPUTE_BIT,
         nullptr}};

    // sets up descriptor set layout and pipeline layout
    RETURN_ON_FATAL_FAILURE(PipelineLayoutTest::SetUp());

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.queueFamilyIndexCount = 1;
    bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.size = bufferSize;

    vkCreateBuffer(device, &bufferCreateInfo, nullptr, &bufferA);
    vkCreateBuffer(device, &bufferCreateInfo, nullptr, &bufferB);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, bufferA, &memoryRequirements);

    DeviceMemoryTest::memorySize = memoryRequirements.size * 2;
    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    vkBindBufferMemory(device, bufferA, memory, 0);
    vkBindBufferMemory(device, bufferB, memory, memoryRequirements.size);

    std::vector<VkDescriptorBufferInfo> bufferInfo{{bufferA, 0, bufferSize},
                                                   {bufferB, 0, bufferSize}};

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = 2;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.dstArrayElement = 0;
    write.dstBinding = 0;
    write.pBufferInfo = bufferInfo.data();
    write.dstSet = descriptorSet;

    vkUpdateDescriptorSets(device, 1, &write, 0, VK_NULL_HANDLE);

    vkGetDeviceQueue(device, 0, 0, &queue);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
  }

  virtual void TearDown() override {
    vkDestroyBuffer(device, bufferA, nullptr);
    vkDestroyBuffer(device, bufferB, nullptr);

    DeviceMemoryTest::TearDown();
    PipelineLayoutTest::TearDown();
    DescriptorPoolTest::TearDown();
    RecordCommandBufferTest::TearDown();
  }

  VkBuffer bufferA, bufferB;
  VkDescriptorSet descriptorSet;
  VkQueue queue;
  VkSubmitInfo submitInfo;
  uint32_t queueFamilyIndex;
  uint32_t bufferSize;
};

TEST_F(CmdBindDescriptorSets, Default) {
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

TEST_F(CmdBindDescriptorSets, DefaultSecondaryCommandBuffer) {
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
  vkCmdBindDescriptorSets(secondaryCommandBuffer,
                          VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
                          &descriptorSet, 0, nullptr);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(secondaryCommandBuffer));
  vkCmdExecuteCommands(commandBuffer, 1, &secondaryCommandBuffer);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  vkFreeCommandBuffers(device, commandPool, 1, &secondaryCommandBuffer);
}

TEST_F(CmdBindDescriptorSets, DefaultDynamicOffset) {
  VkDescriptorSetLayout dynamicDescriptorSetLayout;
  VkDescriptorSet dynamicDescriptorSet;
  VkPipelineLayout dynamicPipelineLayout;

  VkDescriptorSetLayoutBinding binding = {};
  binding.binding = 0;
  binding.descriptorCount = 1;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
  binding.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

  VkDescriptorSetLayoutCreateInfo dSLayoutCreateInfo = {};
  dSLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dSLayoutCreateInfo.pBindings = &binding;
  dSLayoutCreateInfo.bindingCount = 1;

  vkCreateDescriptorSetLayout(device, &dSLayoutCreateInfo, nullptr,
                              &dynamicDescriptorSetLayout);

  VkPipelineLayoutCreateInfo pLayoutCreateInfo = {};
  pLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pLayoutCreateInfo.setLayoutCount = 1;
  pLayoutCreateInfo.pSetLayouts = &dynamicDescriptorSetLayout;

  vkCreatePipelineLayout(device, &pLayoutCreateInfo, nullptr,
                         &dynamicPipelineLayout);

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &dynamicDescriptorSetLayout;

  vkAllocateDescriptorSets(device, &allocInfo, &dynamicDescriptorSet);

  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = bufferA;
  bufferInfo.offset = 0;
  bufferInfo.range = bufferSize;

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
  write.dstArrayElement = 0;
  write.dstBinding = 0;
  write.dstSet = dynamicDescriptorSet;
  write.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  VkPhysicalDeviceProperties properties;

  vkGetPhysicalDeviceProperties(physicalDevice, &properties);

  const uint32_t offset = properties.limits.minStorageBufferOffsetAlignment;

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          dynamicPipelineLayout, 0, 1, &dynamicDescriptorSet, 1,
                          &offset);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  vkDestroyPipelineLayout(device, dynamicPipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, dynamicDescriptorSetLayout, nullptr);
}
