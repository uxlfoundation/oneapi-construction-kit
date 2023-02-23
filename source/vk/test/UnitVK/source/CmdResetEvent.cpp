// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCmdResetEvent

class CmdResetEvent : public uvk::RecordCommandBufferTest {
 public:
  CmdResetEvent()
      : event(VK_NULL_HANDLE), queue(VK_NULL_HANDLE), submitInfo() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(RecordCommandBufferTest::SetUp());

    VkEventCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreateEvent(device, &createInfo, nullptr, &event));

    ASSERT_EQ_RESULT(VK_SUCCESS, vkSetEvent(device, event));

    vkGetDeviceQueue(device, 0, 0, &queue);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
  }

  virtual void TearDown() override {
    vkDestroyEvent(device, event, nullptr);
    RecordCommandBufferTest::TearDown();
  }

  VkEvent event;
  VkQueue queue;
  VkSubmitInfo submitInfo;
};

TEST_F(CmdResetEvent, DefaultDevice) {
  vkCmdResetEvent(commandBuffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_RESET, vkGetEventStatus(device, event));
}

TEST_F(CmdResetEvent, DefaultSecondaryCommandBuffer) {
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
  vkCmdResetEvent(secondaryCommandBuffer, event,
                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(secondaryCommandBuffer));

  vkCmdExecuteCommands(commandBuffer, 1, &secondaryCommandBuffer);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_RESET, vkGetEventStatus(device, event));

  vkFreeCommandBuffers(device, commandPool, 1, &secondaryCommandBuffer);
}

TEST_F(CmdResetEvent, DefaultHost) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetEvent(device, event));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_RESET, vkGetEventStatus(device, event));
}
