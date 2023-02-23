// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkInvalidateMappedMemoryRanges

class InvalidateMappedMemoryRanges : public uvk::PipelineTest,
                                     uvk::DescriptorPoolTest,
                                     uvk::DescriptorSetLayoutTest {
 public:
  // buffer_elements must be two unless shader is changed
  InvalidateMappedMemoryRanges(uint32_t buffer_elements = 2)
      : PipelineTest(uvk::Shader::fill_buffer_2_elems),
        DescriptorPoolTest(true),
        DescriptorSetLayoutTest(true),
        bufferElements(buffer_elements) {}

  virtual void SetUp() override {
    // Set up descriptor set layout
    descriptorSetLayoutBindings.clear();

    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // layout (std430, set=0, binding=0) buffer out { int out[]; };
    layoutBinding.binding = 0;
    descriptorSetLayoutBindings.push_back(layoutBinding);

    RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

    // tell the pipeline create info we want to use this this layout
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 1;

    RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

    // PipelineTest has created our pipeline and shaders for us
    // Time to bind pipeline with the command buffer
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    // create buffer
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeof(uint32_t) * bufferElements;  // size in bytes
    // we will use SSBO or storage buffer so we can read and write
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 1;
    bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
    ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateBuffer(device, &bufferCreateInfo,
                                                nullptr, &bufferOut));

    // sum up memory requirements for our buffers
    VkDeviceSize requiredMemorySize = 0;
    VkMemoryRequirements bufferOutMemoryRequirements;
    vkGetBufferMemoryRequirements(device, bufferOut,
                                  &bufferOutMemoryRequirements);
    bufferOutPhySize = bufferOutMemoryRequirements.size;
    requiredMemorySize = bufferOutPhySize;
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

    uint32_t memoryTypeIndex = 0xffffffff; // (should never be this many types)

    for (uint32_t k = 0; k < memoryProperties.memoryTypeCount; k++) {
      const VkMemoryType memoryType = memoryProperties.memoryTypes[k];
      // need host visible memory but ideally not host coherent if we can find
      // it
      if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        if (memoryTypeIndex == 0xffffffff) memoryTypeIndex = k;
        if (!(memoryType.propertyFlags &
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
          memoryTypeIndex = k;
          using_non_coherent = 1;
          break;
        }
      }
    }

    // allocate on-device memory to match our requirements
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requiredMemorySize;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;
    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkAllocateMemory(device, &allocateInfo, nullptr, &memory));

    // bind buffer to that memory
    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkBindBufferMemory(device, bufferOut, memory, 0));

    // set up the descriptor set
    RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
    vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                             &descriptorSet);

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    VkDescriptorBufferInfo bufferOutInfo = {};
    bufferOutInfo.buffer = bufferOut;
    bufferOutInfo.offset = 0;
    bufferOutInfo.range = VK_WHOLE_SIZE;

    writeDescriptorSet.pBufferInfo = &bufferOutInfo;

    // update the descriptor sets
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDispatch(commandBuffer, 1, 1, 1);
    ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
  }

  virtual void TearDown() override {
    DescriptorPoolTest::TearDown();
    vkDestroyBuffer(device, bufferOut, 0);
    DescriptorSetLayoutTest::TearDown();
    PipelineTest::TearDown();
  }

  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  VkQueue queue;
  VkDeviceMemory memory;

  uint32_t bufferElements;  // in elements, NOT bytes

  VkBuffer bufferOut = VK_NULL_HANDLE;
  VkDeviceSize bufferOutPhySize =
      0;  // actual size occupied by buffer in memory

  uint32_t using_non_coherent = 0;
};

/*
  This tests InvalidateMappedMemoryRanges by doing the following:
  * Maps a region of memory to host
  * Executes the shader (which in this case populates buffer with number 2000)
  * Invalidates the memory
  * Checks results are correct
*/
TEST_F(InvalidateMappedMemoryRanges, Default) {
  // map all the memory to the host (i.e. our memory)
  uint32_t* mapped_data;
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkMapMemory(device, memory, 0, bufferOutPhySize, 0,
                               reinterpret_cast<void**>(&mapped_data)));

  // submit the job
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  // wait for the work to finish
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  // set up a MappedMemoryRange so that Vulkan knows what memory we want to
  // invalidate
  VkMappedMemoryRange flushMappedMemoryRange = {};
  flushMappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  flushMappedMemoryRange.offset = 0;
  flushMappedMemoryRange.pNext = 0;
  flushMappedMemoryRange.size = bufferOutPhySize;
  flushMappedMemoryRange.memory = memory;

  // invalidate from device
  ASSERT_EQ_RESULT(VK_SUCCESS, vkInvalidateMappedMemoryRanges(
                                   device, 1, &flushMappedMemoryRange));

  // validate results
  ASSERT_EQ(mapped_data[0], 2000u)
      << (using_non_coherent ? "using Non-Coherent" : "using Coherent");
  ASSERT_EQ(mapped_data[1], 4000u)
      << (using_non_coherent ? "using Non-Coherent" : "using Coherent");

  vkUnmapMemory(device, memory);
  vkFreeMemory(device, memory, 0);
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
