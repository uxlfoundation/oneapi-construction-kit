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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkUpdateDescriptorSets

class UpdateDescriptorSets : public uvk::PipelineTest,
                             public uvk::DescriptorSetLayoutTest,
                             public uvk::DescriptorPoolTest,
                             public uvk::DeviceMemoryTest {
 public:
  UpdateDescriptorSets()
      : PipelineTest(uvk::Shader::mov),
        DescriptorSetLayoutTest(true),
        DescriptorPoolTest(true),
        DeviceMemoryTest(true),
        bufferA(VK_NULL_HANDLE),
        bufferB(VK_NULL_HANDLE),
        descriptorSet(VK_NULL_HANDLE),
        descriptorSetCopy(VK_NULL_HANDLE),
        submitInfo(),
        numElements(16),
        testVals(numElements, 42) {}

  virtual void SetUp() override {
    descriptorSetLayoutBindings = {{// binding
                                    0,
                                    // type
                                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                    // count
                                    1,
                                    // stage flags
                                    VK_SHADER_STAGE_COMPUTE_BIT,
                                    // immutable samplers
                                    nullptr},
                                   {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                    VK_SHADER_STAGE_COMPUTE_BIT, nullptr}};

    RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

    const std::vector<VkDescriptorPoolSize> sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4}};

    DescriptorPoolTest::poolSizes = sizes;
    RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateDescriptorSets(device, &allocInfo,
                                                          &descriptorSet));

    uint32_t bufferSize = sizeof(uint32_t) * testVals.size();
    const uint32_t queueFamilyIndex = 0;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.queueFamilyIndexCount = 1;
    bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.size = bufferSize;

    vkCreateBuffer(device, &bufferCreateInfo, nullptr, &bufferA);

    vkCreateBuffer(device, &bufferCreateInfo, nullptr, &bufferB);

    VkPhysicalDeviceMemoryProperties memoryProperties;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    VkMemoryRequirements memoryRequirements;

    vkGetBufferMemoryRequirements(device, bufferA, &memoryRequirements);

    bufferBytes = memoryRequirements.size;

    memorySize = bufferBytes * 2;

    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    vkBindBufferMemory(device, bufferA, memory, 0);
    vkBindBufferMemory(device, bufferB, memory, bufferBytes);

    void *memPtr;

    DeviceMemoryTest::mapMemory(0, bufferBytes, &memPtr);
    std::memcpy(memPtr, testVals.data(), testVals.size() * sizeof(uint32_t));
    DeviceMemoryTest::unmapMemory();

    bufferInfo = {{bufferA, 0, bufferSize}, {bufferB, 0, bufferSize}};

    VkWriteDescriptorSet write = {};

    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.dstArrayElement = 0;
    write.dstBinding = 0;
    write.pBufferInfo =
        &bufferInfo[0];  // NOLINT(readability-container-data-pointer)
    write.dstSet = descriptorSet;

    writes.push_back(write);

    write.dstBinding = 1;
    write.pBufferInfo = &bufferInfo[1];

    writes.push_back(write);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkGetDeviceQueue(device, 0, 0, &queue);
  }

  virtual void TearDown() override {
    vkDestroyBuffer(device, bufferA, nullptr);
    vkDestroyBuffer(device, bufferB, nullptr);

    DescriptorSetLayoutTest::TearDown();
    DescriptorPoolTest::TearDown();
    DeviceMemoryTest::TearDown();
    PipelineTest::TearDown();
  }

  VkBuffer bufferA, bufferB;
  uint32_t bufferBytes;
  VkDescriptorSet descriptorSet, descriptorSetCopy;
  VkQueue queue;
  VkSubmitInfo submitInfo;
  uint32_t numElements;
  std::vector<uint32_t> testVals;
  std::vector<VkDescriptorBufferInfo> bufferInfo;
  std::vector<VkWriteDescriptorSet> writes;
};

TEST_F(UpdateDescriptorSets, Default) {
  vkUpdateDescriptorSets(device, 2, writes.data(), 0, nullptr);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

  vkCmdDispatch(commandBuffer, 1, 1, numElements);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  // verify the descriptor sets were updated by checking that the test value has
  // moved from one buffer to the other when the kernel was run
  void *memPtr;
  DeviceMemoryTest::mapMemory(bufferBytes, bufferBytes, &memPtr);
  for (uint32_t testValIndex = 0; testValIndex < numElements; testValIndex++) {
    ASSERT_EQ(testVals[testValIndex],
              reinterpret_cast<uint32_t *>(memPtr)[testValIndex]);
  }
  DeviceMemoryTest::unmapMemory();
}

TEST_F(UpdateDescriptorSets, DefaultWriteOverflow) {
  // tests overflowing a write into the next binding if the specified binding is
  // out of descriptors

  VkWriteDescriptorSet write = writes[0];
  write.descriptorCount = 2;
  write.pBufferInfo = bufferInfo.data();

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

  vkCmdDispatch(commandBuffer, 1, 1, numElements);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  // verify the descriptor sets were updated by checking that the test value has
  // moved from one buffer to the other when the kernel was run
  void *memPtr;
  DeviceMemoryTest::mapMemory(bufferBytes, bufferBytes, &memPtr);
  for (uint32_t testValIndex = 0; testValIndex < numElements; testValIndex++) {
    ASSERT_EQ(testVals[testValIndex],
              reinterpret_cast<uint32_t *>(memPtr)[testValIndex]);
  }
  DeviceMemoryTest::unmapMemory();
}

TEST_F(UpdateDescriptorSets, DefaultCopy) {
  vkUpdateDescriptorSets(device, 2, writes.data(), 0, nullptr);

  VkDescriptorSet descriptorSetCopy;

  VkDescriptorSetAllocateInfo dSetCopyAllocInf = {};
  dSetCopyAllocInf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dSetCopyAllocInf.descriptorPool = descriptorPool;
  dSetCopyAllocInf.descriptorSetCount = 1;
  dSetCopyAllocInf.pSetLayouts = &descriptorSetLayout;

  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkAllocateDescriptorSets(device, &dSetCopyAllocInf, &descriptorSetCopy));

  std::vector<VkCopyDescriptorSet> copies;

  VkCopyDescriptorSet copy = {};
  copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
  copy.descriptorCount = 1;
  copy.dstArrayElement = 0;
  copy.srcArrayElement = 0;
  copy.dstBinding = 0;
  copy.srcBinding = 0;
  copy.dstSet = descriptorSetCopy;
  copy.srcSet = descriptorSet;

  copies.push_back(copy);

  copy.srcBinding = 1;
  copy.dstBinding = 1;

  copies.push_back(copy);

  vkUpdateDescriptorSets(device, 0, nullptr, 2, copies.data());

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSetCopy, 0, nullptr);

  vkCmdDispatch(commandBuffer, 1, 1, numElements);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  // verify the descriptor sets were updated by checking that the test value has
  // moved from one buffer to the other when the kernel was run
  void *memPtr;
  DeviceMemoryTest::mapMemory(bufferBytes, bufferBytes, &memPtr);
  for (uint32_t testValIndex = 0; testValIndex < numElements; testValIndex++) {
    ASSERT_EQ(testVals[testValIndex],
              reinterpret_cast<uint32_t *>(memPtr)[testValIndex]);
  }
  DeviceMemoryTest::unmapMemory();
}

TEST_F(UpdateDescriptorSets, DefaultCopyOverflow) {
  // same as write overflow but for copy
  vkUpdateDescriptorSets(device, 2, writes.data(), 0, nullptr);

  VkDescriptorSet descriptorSetCopy;

  VkDescriptorSetAllocateInfo dSetCopyAllocInf = {};
  dSetCopyAllocInf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dSetCopyAllocInf.descriptorPool = descriptorPool;
  dSetCopyAllocInf.descriptorSetCount = 1;
  dSetCopyAllocInf.pSetLayouts = &descriptorSetLayout;

  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkAllocateDescriptorSets(device, &dSetCopyAllocInf, &descriptorSetCopy));

  VkCopyDescriptorSet copy = {};
  copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
  copy.descriptorCount = 2;
  copy.dstArrayElement = 0;
  copy.srcArrayElement = 0;
  copy.dstBinding = 0;
  copy.srcBinding = 0;
  copy.dstSet = descriptorSetCopy;
  copy.srcSet = descriptorSet;

  vkUpdateDescriptorSets(device, 0, nullptr, 1, &copy);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSetCopy, 0, nullptr);

  vkCmdDispatch(commandBuffer, 1, 1, numElements);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  // verify the descriptor sets were updated by checking that the test value has
  // moved from one buffer to the other when the kernel was run
  void *memPtr;
  DeviceMemoryTest::mapMemory(bufferBytes, bufferBytes, &memPtr);
  for (uint32_t testValIndex = 0; testValIndex < numElements; testValIndex++) {
    ASSERT_EQ(testVals[testValIndex],
              reinterpret_cast<uint32_t *>(memPtr)[testValIndex]);
  }
  DeviceMemoryTest::unmapMemory();
}

TEST_F(UpdateDescriptorSets, DefaultSecondaryCommandBuffer) {
  vkUpdateDescriptorSets(device, 2, writes.data(), 0, nullptr);

  VkCommandBufferAllocateInfo allocInf = {};
  allocInf.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInf.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  allocInf.commandPool = commandPool;
  allocInf.commandBufferCount = 1;

  VkCommandBuffer secondaryCommandBuffer;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateCommandBuffers(
                                   device, &allocInf, &secondaryCommandBuffer));

  VkCommandBufferInheritanceInfo inheritanceInfo = {};
  inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = &inheritanceInfo;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkBeginCommandBuffer(secondaryCommandBuffer, &beginInfo));

  vkCmdBindDescriptorSets(secondaryCommandBuffer,
                          VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
                          &descriptorSet, 0, nullptr);

  vkCmdBindPipeline(secondaryCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline);

  vkCmdDispatch(secondaryCommandBuffer, 1, 1, numElements);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(secondaryCommandBuffer));

  vkCmdExecuteCommands(commandBuffer, 1, &secondaryCommandBuffer);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  // verify the descriptor sets were updated by checking that the test value has
  // moved from one buffer to the other when the kernel was run
  void *memPtr;
  DeviceMemoryTest::mapMemory(bufferBytes, bufferBytes, &memPtr);
  for (uint32_t testValIndex = 0; testValIndex < numElements; testValIndex++) {
    ASSERT_EQ(testVals[testValIndex],
              reinterpret_cast<uint32_t *>(memPtr)[testValIndex]);
  }
  DeviceMemoryTest::unmapMemory();

  vkFreeCommandBuffers(device, commandPool, 1, &secondaryCommandBuffer);
}
