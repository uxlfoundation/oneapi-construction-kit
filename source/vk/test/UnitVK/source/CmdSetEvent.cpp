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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCmdSetEvent

class CmdSetEvent : public uvk::RecordCommandBufferTest {
 public:
  CmdSetEvent() : event(VK_NULL_HANDLE), submitInfo() {}

  virtual void SetUp() {
    RETURN_ON_FATAL_FAILURE(RecordCommandBufferTest::SetUp());

    VkEventCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreateEvent(device, &createInfo, nullptr, &event));

    vkGetDeviceQueue(device, 0, 0, &queue);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
  }

  virtual void TearDown() {
    vkDestroyEvent(device, event, nullptr);
    RecordCommandBufferTest::TearDown();
  }

  VkEvent event;
  VkQueue queue;
  VkSubmitInfo submitInfo;
};

TEST_F(CmdSetEvent, DefaultDevice) {
  vkCmdSetEvent(commandBuffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));
}

TEST_F(CmdSetEvent, DefaultSecondaryCommandBuffer) {
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
  vkCmdSetEvent(secondaryCommandBuffer, event,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(secondaryCommandBuffer));

  vkCmdExecuteCommands(commandBuffer, 1, &secondaryCommandBuffer);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));

  vkFreeCommandBuffers(device, commandPool, 1, &secondaryCommandBuffer);
}

TEST_F(CmdSetEvent, DefaultHost) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkSetEvent(device, event));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));
}

TEST_F(CmdSetEvent, DoubleSet) {
  vkCmdSetEvent(commandBuffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkSetEvent(device, event));
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));
}

TEST_F(CmdSetEvent, HostReset) {
  vkCmdSetEvent(commandBuffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetEvent(device, event));

  ASSERT_EQ_RESULT(VK_EVENT_RESET, vkGetEventStatus(device, event));
}

TEST_F(CmdSetEvent, HDH) {
  vkCmdResetEvent(commandBuffer, event, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkSetEvent(device, event));
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkSetEvent(device, event));

  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));
}
