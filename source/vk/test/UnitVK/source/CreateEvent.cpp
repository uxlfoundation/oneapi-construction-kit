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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateEvent

class CreateEvent : public uvk::DeviceTest {
 public:
  CreateEvent() : createInfo(), event(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
  }

  virtual void TearDown() override {
    if (event) {
      vkDestroyEvent(device, event, nullptr);
    }

    DeviceTest::TearDown();
  }

  VkEventCreateInfo createInfo;
  VkEvent event;
};

TEST_F(CreateEvent, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateEvent(device, &createInfo, nullptr, &event));
}

TEST_F(CreateEvent, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateEvent(device, &createInfo,
                                             uvk::defaultAllocator(), &event));
  vkDestroyEvent(device, event, uvk::defaultAllocator());
  event = VK_NULL_HANDLE;
}

TEST_F(CreateEvent, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateEvent(device, &createInfo, uvk::nullAllocator(), &event));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
