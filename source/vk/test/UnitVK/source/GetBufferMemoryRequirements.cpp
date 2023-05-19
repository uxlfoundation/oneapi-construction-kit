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

class GetBufferMemoryRequirements : public uvk::DeviceTest {
public:
  GetBufferMemoryRequirements()
      : queueFamilyIndex(0), bufferSize(64), bufferCreateInfo(),
        buffer(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.queueFamilyIndexCount = 1;
    bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.size = bufferSize;
  }

  virtual void TearDown() override {
    vkDestroyBuffer(device, buffer, nullptr);
    DeviceTest::TearDown();
  }

  uint32_t queueFamilyIndex;
  uint32_t bufferSize;
  VkBufferCreateInfo bufferCreateInfo;
  VkBuffer buffer;
};

TEST_F(GetBufferMemoryRequirements, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer));

  VkMemoryRequirements memoryRequirements;

  vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

  // since the function doesn't return a value carry out some sanity checks on
  // the returned requirements
  ASSERT_NE(0u, memoryRequirements.memoryTypeBits);
  ASSERT_TRUE(memoryRequirements.size >= bufferSize);
}

TEST_F(GetBufferMemoryRequirements, DefaultForceRoundUp) {
  bufferCreateInfo.size = 150;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer));

  VkMemoryRequirements memoryRequirements;

  vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

  // since the function doesn't return a value carry out some sanity checks on
  // the returned requirements
  ASSERT_NE(0u, memoryRequirements.memoryTypeBits);
  ASSERT_TRUE(memoryRequirements.size >= bufferSize);
}
