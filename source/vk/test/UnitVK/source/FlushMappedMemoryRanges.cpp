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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkFlushMappedMemoryRanges

class FlushMappedMemoryRanges : public uvk::PipelineTest,
                                uvk::DescriptorPoolTest,
                                uvk::DescriptorSetLayoutTest,
                                uvk::BufferTest {
 public:
  FlushMappedMemoryRanges()
      : PipelineTest(uvk::Shader::mov),
        DescriptorPoolTest(true),
        DescriptorSetLayoutTest(true),
        BufferTest(sizeof(uint32_t) * bufferElements,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true) {}

  virtual void SetUp() {
    // Set up the descriptor set layout
    descriptorSetLayoutBindings.clear();

    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // layout (std430, set=0, binding=0) buffer in { int in[]; };
    layoutBinding.binding = 0;
    descriptorSetLayoutBindings.push_back(layoutBinding);

    // layout (std430, set=0, binding=1) buffer out { int out[]; }; (output
    // buffer)
    layoutBinding.binding = 1;
    descriptorSetLayoutBindings.push_back(layoutBinding);

    RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

    // tell the pipeline create info we want to use this this layout
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 1;

    RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

    // PipelineTest has created our pipeline and shaders for us
    // Time to bind pipeline with the command buffer
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());
    ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateBuffer(device, &bufferCreateInfo,
                                                nullptr, &buffer2));

    alignedBufferSize = alignedDeviceSize(bufferMemoryRequirements);
    totalMemorySize = alignedBufferSize * 2;

    // now we need to get device memory.

    // Note that the standard states that there must be at least one memory type
    // with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT set,
    // but there is no requirement that a memory type is non-coherent. See
    // https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#kPhysicalDeviceMemoryProperties

    // the properties we desire are host visible and non-coherant
    // but, if there is no non-coherant memory, we can test with
    // just host visible
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    uint32_t memoryTypeIndex = 0xffffffff;  // (should never be this many types)

    for (uint32_t k = 0; k < memoryProperties.memoryTypeCount; k++) {
      const VkMemoryType memoryType = memoryProperties.memoryTypes[k];
      // need host visible memory but ideally not host coherent if we can find
      // it
      if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        if (memoryTypeIndex == 0xffffffff) memoryTypeIndex = k;
        if (!(memoryType.propertyFlags &
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
          memoryTypeIndex = k;
          break;
        }
      }
    }

    // allocate on-device memory to match our requirements
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = totalMemorySize;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;
    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkAllocateMemory(device, &allocateInfo, nullptr, &memory));

    // bind our two buffers to the memory
    // binding bufferIn, start address is beginning of memory
    ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer, memory, 0));

    // bind bufferOut to memory, offsetted after the input buffer
    ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer2, memory,
                                                    alignedBufferSize));

    // Set up the descriptor pool so we can actually allocate ourselves a
    // descriptor set
    RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

    // Allocate ourselves a descriptor set, which we can use
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    // this is the same layout we used to describe to the pipeline which
    // descriptors will be used
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
    ASSERT_EQ_RESULT(
        VK_SUCCESS, vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                             &descriptorSet));

    // Next bit of code updates descriptor sets so that the shader knows where
    // our data
    // is bound (i.e. we are passing paramaters, in this case the buffers, to
    // the device)
    // now we need to update the descriptor set telling it about our buffers
    std::vector<VkWriteDescriptorSet> descriptorSetWrites;
    // we can reuse this structure as it will be copied each time we push to the
    // vector of descriptor set writes
    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    // each buffer needs its own buffer info as it is passed as a pointer
    VkDescriptorBufferInfo bufferInInfo = {};
    bufferInInfo.buffer = buffer;
    bufferInInfo.offset = 0;
    bufferInInfo.range = VK_WHOLE_SIZE;

    // Push write descriptor set for bufferIn
    writeDescriptorSet.pBufferInfo = &bufferInInfo;
    descriptorSetWrites.push_back(writeDescriptorSet);

    VkDescriptorBufferInfo bufferOutInfo = {};
    bufferOutInfo.buffer = buffer2;
    bufferOutInfo.offset = 0;
    bufferOutInfo.range = VK_WHOLE_SIZE;

    // Push write descriptor set for bufferOut with corresponding bindings
    writeDescriptorSet.dstBinding = 1;
    writeDescriptorSet.pBufferInfo = &bufferOutInfo;
    descriptorSetWrites.push_back(writeDescriptorSet);

    // update the descriptor sets
    vkUpdateDescriptorSets(device, descriptorSetWrites.size(),
                           descriptorSetWrites.data(), 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDispatch(commandBuffer, 1, 1, bufferElements);
    ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

    // get a queue handle
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    // we are now ready to mess with memory and execute the shader
  }

  virtual void TearDown() {
    vkFreeMemory(device, memory, nullptr);
    vkDestroyBuffer(device, buffer2, nullptr);
    BufferTest::TearDown();
    DescriptorPoolTest::TearDown();
    DescriptorSetLayoutTest::TearDown();
    PipelineTest::TearDown();
  }

  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  VkQueue queue;
  VkDeviceMemory memory;

  static const uint32_t bufferElements = 16;  // in elements, NOT bytes

  VkBuffer buffer2 = VK_NULL_HANDLE;

  VkDeviceSize alignedBufferSize, totalMemorySize = 0;
};

/*
This tests FlushMappedMemoryRanges by doing the following:
* Prepare a pipeline with a simple 1d buffer copy shader
* Maps the memory to host and fills input buffer with random data
* Flushes memory to device
* Executes the shader
* Invalidates the memory to read back from device
* Compares result buffer to the random data

  Note: Due to global variables not yet being implemented in SPIRV, the shader
        that is currently executed (mov_buffer_first_elem) simply copies the
        first element in the buffer instead of of the whole buffer
        (mov_1d_buffer)

  TODO: Change the shader to mov_1d_buffer once GlobalInvocationID implemented
*/
TEST_F(FlushMappedMemoryRanges, Default) {
  // The SetUp() function at this point has done the following:
  // * Created two buffers and allocated them into device memory
  // * Allocated one block of memory to store both buffers, ideally using
  // coherent memory
  // * Set up a pipeline with our shader - which is simply a memory copy (see
  // TODO, above)
  // * Recorded our commands into a command buffer

  // Now need to write to memory and try using flush()

  // map all the memory to the host (i.e. our memory)
  void *mappedMemory;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0,
                                           &mappedMemory));

  // Vulkan API standard states that for non-coherent memory the mapped memory
  // must first be invalidated before it is written to
  // but only if device writes have been made?
  // see the info box @:
  // www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#vkFlushMappedMemoryRanges
  // to quote: Mapping non-coherent memory does not implicitly invalidate the
  // mapped memory,
  // and device writes that have not been invalidated must be made visible
  // before the host reads or overwrites them.

  // vector to store test data: used to compare results
  std::vector<uint32_t> testData;

  // fill input buffer with random data: we will also keep a local copy so that
  // we can verify the results
  srand(std::time(NULL));
  for (uint32_t k = 0; k < bufferElements; k++) {
    const uint32_t random_no = static_cast<uint32_t>(rand());
    static_cast<uint32_t *>(mappedMemory)[k] = random_no;
    testData.push_back(random_no);
  }

  // set up a MappedMemoryRange so that Vulkan knows what memory we want to
  // flush, in this case all of the memory which contains both buffers
  VkMappedMemoryRange flushMappedMemoryRange = {};
  flushMappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  flushMappedMemoryRange.offset = 0;
  flushMappedMemoryRange.pNext = 0;
  flushMappedMemoryRange.size = totalMemorySize;
  flushMappedMemoryRange.memory = memory;

  // flush to device
  ASSERT_EQ_RESULT(VK_SUCCESS, vkFlushMappedMemoryRanges(
                                   device, 1, &flushMappedMemoryRange));

  // now that the data has been sent to the device, fire away the work!
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  // wait for the work to finish
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  // read back from the device, in our case simply by invalidating the same
  // memory region used before
  ASSERT_EQ_RESULT(VK_SUCCESS, vkInvalidateMappedMemoryRanges(
                                   device, 1, &flushMappedMemoryRange));

  uint32_t *resultMemory = static_cast<uint32_t *>(mappedMemory) +
                           (alignedBufferSize / sizeof(uint32_t));

  // validate results
  for (uint32_t k = 0; k < bufferElements; k++) {
    // Check that the output buffer now has the correct data
    ASSERT_EQ(resultMemory[k], testData[k]);
  }

  for (uint32_t k = 0; k < bufferElements; k++) {
    // Check that the input buffer still has test data
    ASSERT_EQ(static_cast<uint32_t *>(mappedMemory)[k], testData[k]);
  }
  // unmap the memory
  vkUnmapMemory(device, memory);
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
