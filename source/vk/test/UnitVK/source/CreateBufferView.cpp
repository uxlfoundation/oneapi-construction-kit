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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateBufferView

class CreateBufferView : public uvk::BufferTest, public uvk::DeviceMemoryTest {
 public:
  CreateBufferView()
      : BufferTest(128, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT),
        DeviceMemoryTest(true),
        bufferViewCreateInfo(),
        bufferView(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

    DeviceMemoryTest::memorySize = BufferTest::bufferMemoryRequirements.size;
    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    vkBindBufferMemory(device, buffer, memory, 0);

    bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    bufferViewCreateInfo.range = VK_WHOLE_SIZE;
    bufferViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    bufferViewCreateInfo.buffer = buffer;
  }

  virtual void TearDown() override {
    if (bufferView) {
      vkDestroyBufferView(device, bufferView, nullptr);
    }

    DeviceMemoryTest::TearDown();
    BufferTest::TearDown();
  }

  VkBufferViewCreateInfo bufferViewCreateInfo;
  VkBufferView bufferView;
};

TEST_F(CreateBufferView, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateBufferView(device, &bufferViewCreateInfo,
                                                  nullptr, &bufferView));
}

TEST_F(CreateBufferView, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateBufferView(device, &bufferViewCreateInfo,
                                      uvk::defaultAllocator(), &bufferView));

  vkDestroyBufferView(device, bufferView, uvk::defaultAllocator());
  bufferView = VK_NULL_HANDLE;
}

// TODO: a test which pushes one of these in a descriptor set update

TEST_F(CreateBufferView, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(VK_ERROR_OUT_OF_HOST_MEMORY,
                   vkCreateBufferView(device, &bufferViewCreateInfo,
                                      uvk::nullAllocator(), &bufferView));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
