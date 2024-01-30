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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCmdDispatch

class CmdDispatch : public uvk::PipelineTest,
                    public uvk::DescriptorSetLayoutTest,
                    public uvk::DescriptorPoolTest,
                    public uvk::DeviceMemoryTest,
                    public uvk::BufferTest {
 public:
  CmdDispatch()
      : DescriptorSetLayoutTest(true),
        DescriptorPoolTest(true),
        DeviceMemoryTest(true),
        BufferTest(0,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   true),
        submitInfo() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    vkGetDeviceQueue(device, 0, 0, &queue);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
  }

  virtual void TearDown() override { PipelineTest::TearDown(); }

  VkQueue queue;
  VkSubmitInfo submitInfo;
};

TEST_F(CmdDispatch, Default) {
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
}

TEST_F(CmdDispatch, DefaultSecondaryCommandBuffer) {
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

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
  vkCmdBindPipeline(secondaryCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline);
  vkCmdDispatch(secondaryCommandBuffer, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(secondaryCommandBuffer));

  vkCmdExecuteCommands(commandBuffer, 1, &secondaryCommandBuffer);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  vkFreeCommandBuffers(device, commandPool, 1, &secondaryCommandBuffer);
}

TEST_F(CmdDispatch, DefaultSpecializationConstant) {
  descriptorSetLayoutBindings = {{
      0,                                  // binding
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // type
      1,                                  // count
      VK_SHADER_STAGE_COMPUTE_BIT,        // stage flags
      nullptr                             // immutable samplers
  }};

  RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

  uint32_t specConstant = 42;

  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

  PipelineTest::shader = uvk::Shader::spec_const;

  VkSpecializationMapEntry entry = {};
  entry.offset = 0;
  entry.size = sizeof(specConstant);
  entry.constantID = 0;

  VkSpecializationInfo specInfo = {};
  specInfo.dataSize = sizeof(specConstant);
  specInfo.mapEntryCount = 1;
  specInfo.pMapEntries = &entry;
  specInfo.pData = &specConstant;

  PipelineTest::pSpecializationInfo = &specInfo;

  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

  VkDescriptorSet descriptorSet;

  VkDescriptorSetAllocateInfo dSetAllocInfo = {};
  dSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dSetAllocInfo.descriptorPool = descriptorPool;
  dSetAllocInfo.pSetLayouts = &descriptorSetLayout;
  dSetAllocInfo.descriptorSetCount = 1;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateDescriptorSets(device, &dSetAllocInfo,
                                                        &descriptorSet));

  bufferSize = sizeof(specConstant);
  RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

  memorySize = bufferMemoryRequirements.size;

  RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer, memory, 0));

  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = buffer;
  bufferInfo.offset = 0;
  bufferInfo.range = VK_WHOLE_SIZE;

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write.dstBinding = 0;
  write.dstArrayElement = 0;
  write.dstSet = descriptorSet;
  write.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *memPtr;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &memPtr);

  // the test kernel adds a constant value to the spec constant to test for a
  // bug where specializing spec constants can overwrite other constants with
  // the same value as the spec constant's default value
  ASSERT_EQ(specConstant + 24, *reinterpret_cast<uint32_t *>(memPtr));

  DeviceMemoryTest::unmapMemory();

  BufferTest::TearDown();
  DeviceMemoryTest::TearDown();
  DescriptorPoolTest::TearDown();
  DescriptorSetLayoutTest::TearDown();
}

TEST_F(CmdDispatch, DefaultRuntimeArray) {
  descriptorSetLayoutBindings = {
      {
          0,                                  // binding
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // type
          1,                                  // count
          VK_SHADER_STAGE_COMPUTE_BIT,        // stage flags
          nullptr                             // immutable_samplers
      },
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
       nullptr}};

  RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

  PipelineTest::shader = uvk::Shader::runtime_array;
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  bufferSize = sizeof(float) + sizeof(int);

  RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

  bufferSize = sizeof(int);

  const uint32_t inBufferRequiredSize = bufferMemoryRequirements.size;

  VkBuffer outBuffer;
  vkCreateBuffer(device, &bufferCreateInfo, nullptr, &outBuffer);

  vkGetBufferMemoryRequirements(device, outBuffer, &bufferMemoryRequirements);

  DeviceMemoryTest::memorySize =
      bufferMemoryRequirements.size + inBufferRequiredSize;
  RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

  vkBindBufferMemory(device, buffer, memory, 0);

  vkBindBufferMemory(device, outBuffer, memory, inBufferRequiredSize);

  RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

  VkDescriptorSet descriptorSet;

  VkDescriptorSetAllocateInfo dSetAllocInf = {};
  dSetAllocInf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dSetAllocInf.descriptorSetCount = 1;
  dSetAllocInf.descriptorPool = descriptorPool;
  dSetAllocInf.pSetLayouts = &descriptorSetLayout;

  vkAllocateDescriptorSets(device, &dSetAllocInf, &descriptorSet);

  std::vector<VkDescriptorBufferInfo> bufferInfos = {
      {buffer, 0, VK_WHOLE_SIZE}, {outBuffer, 0, VK_WHOLE_SIZE}};

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = descriptorSet;
  write.dstBinding = 0;
  write.dstArrayElement = 0;
  write.descriptorCount = 2;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write.pBufferInfo = bufferInfos.data();

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  void *mappedMemory;

  const uint32_t initialOutValue = 0;

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdFillBuffer(commandBuffer, outBuffer, 0, sizeof(initialOutValue),
                  initialOutValue);

  // Add a pipeline barrier before dispatching any compute commands to ensure
  // the transfer command FillBuffer finishes first
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                       VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE);

  vkCmdDispatch(commandBuffer, 1, 1, 1);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

  vkQueueWaitIdle(queue);

  DeviceMemoryTest::mapMemory(inBufferRequiredSize,
                              bufferMemoryRequirements.size, &mappedMemory);

  // we allocated enough additional space in the buffer for one additional
  // value, so this is what we expect the output to be
  ASSERT_EQ(*static_cast<uint32_t *>(mappedMemory), 1u);

  DeviceMemoryTest::unmapMemory();

  vkDestroyBuffer(device, outBuffer, nullptr);

  BufferTest::TearDown();
  DeviceMemoryTest::TearDown();
  DescriptorPoolTest::TearDown();
  DescriptorSetLayoutTest::TearDown();
}

TEST_F(CmdDispatch, glNumWorkGroups) {
  RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

  PipelineTest::shader = uvk::Shader::num_work_groups;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  // we want a buffer that can hold all three dimensions of the work group
  bufferSize = sizeof(int) * 3;

  RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

  memorySize = BufferTest::bufferMemoryRequirements.size;
  RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer, memory, 0));

  RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

  VkDescriptorSet descriptor_set;

  VkDescriptorSetAllocateInfo dSetAllocInf = {};
  dSetAllocInf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dSetAllocInf.descriptorPool = descriptorPool;
  dSetAllocInf.descriptorSetCount = 1;
  dSetAllocInf.pSetLayouts = &descriptorSetLayout;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateDescriptorSets(device, &dSetAllocInf,
                                                        &descriptor_set));

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
  write.dstSet = descriptor_set;
  write.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptor_set, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  // the values we give as dimensions here will be what we get back from
  // gl_NumWorkGroups
  std::array<uint32_t, 3> numWorkGroups = {42, 1, 24};
  vkCmdDispatch(commandBuffer, numWorkGroups[0], numWorkGroups[1],
                numWorkGroups[2]);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  for (int memIndex = 0; memIndex < 3; memIndex++) {
    ASSERT_EQ(reinterpret_cast<uint32_t *>(mappedMemory)[memIndex],
              numWorkGroups[memIndex]);
  }

  DeviceMemoryTest::unmapMemory();

  BufferTest::TearDown();
  DeviceMemoryTest::TearDown();
  DescriptorPoolTest::TearDown();
  DescriptorSetLayoutTest::TearDown();
}

TEST_F(CmdDispatch, glWorkGroupID) {
  RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

  PipelineTest::shader = uvk::Shader::work_group_id;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  // here we need a buffer big enough to hold a value for each invocation
  bufferSize = sizeof(int) * 8;

  RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

  memorySize = BufferTest::bufferMemoryRequirements.size;
  RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer, memory, 0));

  RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

  VkDescriptorSet descriptor_set;

  VkDescriptorSetAllocateInfo dSetAllocInf = {};
  dSetAllocInf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dSetAllocInf.descriptorPool = descriptorPool;
  dSetAllocInf.descriptorSetCount = 1;
  dSetAllocInf.pSetLayouts = &descriptorSetLayout;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateDescriptorSets(device, &dSetAllocInf,
                                                        &descriptor_set));

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
  write.dstSet = descriptor_set;
  write.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptor_set, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 8, 1, 1);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  for (int mem_index = 0; mem_index < 8; mem_index++) {
    // since our global size is 8 and our local size (defined in the kernel) is
    // four, our first four IDs will be 0 and the last four will be 1
    const uint32_t expected_id = mem_index < 4 ? 0 : 1;
    ASSERT_EQ(reinterpret_cast<uint32_t *>(mappedMemory)[mem_index],
              expected_id);
  }

  DeviceMemoryTest::unmapMemory();

  BufferTest::TearDown();
  DeviceMemoryTest::TearDown();
  DescriptorPoolTest::TearDown();
  DescriptorSetLayoutTest::TearDown();
}

TEST_F(CmdDispatch, glLocalInvocationID) {
  RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

  PipelineTest::shader = uvk::Shader::local_invocation_id;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  // here we need a buffer big enough to hold a value for each invocation
  bufferSize = sizeof(int) * 8;

  RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

  memorySize = BufferTest::bufferMemoryRequirements.size;
  RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer, memory, 0));

  RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

  VkDescriptorSet descriptor_set;

  VkDescriptorSetAllocateInfo dSetAllocInf = {};
  dSetAllocInf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dSetAllocInf.descriptorPool = descriptorPool;
  dSetAllocInf.descriptorSetCount = 1;
  dSetAllocInf.pSetLayouts = &descriptorSetLayout;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateDescriptorSets(device, &dSetAllocInf,
                                                        &descriptor_set));

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
  write.dstSet = descriptor_set;
  write.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptor_set, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 8, 1, 1);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  for (int mem_index = 0; mem_index < 8; mem_index++) {
    // since our global size is 8 and our local size (defined in the kernel) is
    // four, our output will be numbers 0-3 twice, once for each work group
    const uint32_t expected_id = mem_index % 4;
    ASSERT_EQ(reinterpret_cast<uint32_t *>(mappedMemory)[mem_index],
              expected_id);
  }

  DeviceMemoryTest::unmapMemory();

  BufferTest::TearDown();
  DeviceMemoryTest::TearDown();
  DescriptorPoolTest::TearDown();
  DescriptorSetLayoutTest::TearDown();
}

TEST_F(CmdDispatch, glGlobalInvocationID) {
  RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

  PipelineTest::shader = uvk::Shader::global_invocation_id;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  // here we need a buffer big enough to hold a value for each invocation
  bufferSize = sizeof(int) * 8;

  RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

  memorySize = BufferTest::bufferMemoryRequirements.size;
  RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer, memory, 0));

  RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

  VkDescriptorSet descriptor_set;

  VkDescriptorSetAllocateInfo dSetAllocInf = {};
  dSetAllocInf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dSetAllocInf.descriptorPool = descriptorPool;
  dSetAllocInf.descriptorSetCount = 1;
  dSetAllocInf.pSetLayouts = &descriptorSetLayout;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateDescriptorSets(device, &dSetAllocInf,
                                                        &descriptor_set));

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
  write.dstSet = descriptor_set;
  write.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptor_set, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 8, 1, 1);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  for (uint32_t mem_index = 0; mem_index < 8; mem_index++) {
    // here we expect the output to be a list of zero to dispatch x dimension-1
    ASSERT_EQ(reinterpret_cast<uint32_t *>(mappedMemory)[mem_index], mem_index);
  }

  DeviceMemoryTest::unmapMemory();

  BufferTest::TearDown();
  DeviceMemoryTest::TearDown();
  DescriptorPoolTest::TearDown();
  DescriptorSetLayoutTest::TearDown();
}

TEST_F(CmdDispatch, glLocalInvocationIndex) {
  RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

  PipelineTest::shader = uvk::Shader::local_invocation_index;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
  RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

  // the local work group sizes
  const int width = 4, height = 4;

  // here we need a buffer big enough to hold a value for each invocation
  bufferSize = sizeof(int) * width * height;

  RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

  memorySize = BufferTest::bufferMemoryRequirements.size;
  RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer, memory, 0));

  RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

  VkDescriptorSet descriptor_set;

  VkDescriptorSetAllocateInfo dSetAllocInf = {};
  dSetAllocInf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dSetAllocInf.descriptorPool = descriptorPool;
  dSetAllocInf.descriptorSetCount = 1;
  dSetAllocInf.pSetLayouts = &descriptorSetLayout;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateDescriptorSets(device, &dSetAllocInf,
                                                        &descriptor_set));

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
  write.dstSet = descriptor_set;
  write.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptor_set, 0, nullptr);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdDispatch(commandBuffer, 1, 1, 1);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &commandBuffer;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  void *mappedMemory;

  DeviceMemoryTest::mapMemory(0, VK_WHOLE_SIZE, &mappedMemory);

  // here we again expect the numbers zero to n-1, this time in a 4x4 grid
  // pattern (hence the nested loop to read) as the output is being written with
  // the x and y values of global invocation ID
  uint32_t expected = 0;
  for (int mem_y = 0; mem_y < height; mem_y++) {
    for (int mem_x = 0; mem_x < width; mem_x++) {
      const int index = (mem_y * width) + mem_x;
      EXPECT_EQ(reinterpret_cast<uint32_t *>(mappedMemory)[index], expected);
      expected++;
    }
  }

  DeviceMemoryTest::unmapMemory();

  BufferTest::TearDown();
  DeviceMemoryTest::TearDown();
  DescriptorPoolTest::TearDown();
  DescriptorSetLayoutTest::TearDown();
}
