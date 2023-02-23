// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkBeginCommandBuffer

class BeginCommandBuffer : public uvk::CommandPoolTest {
public:
  BeginCommandBuffer() : commandBuffer(VK_NULL_HANDLE), beginInfo() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(CommandPoolTest::SetUp());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  }

  virtual void TearDown() override {
    if (commandBuffer) {
      vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    CommandPoolTest::TearDown();
  }

  VkCommandBuffer commandBuffer;
  VkCommandBufferBeginInfo beginInfo;
};

TEST_F(BeginCommandBuffer, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkBeginCommandBuffer(commandBuffer, &beginInfo));
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
