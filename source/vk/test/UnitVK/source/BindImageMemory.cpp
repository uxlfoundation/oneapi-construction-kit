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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkBindImageMemory

class BindImageMemoryTest : public uvk::DeviceTest {
 public:
  BindImageMemoryTest()
      : image(VK_NULL_HANDLE), deviceMemory(VK_NULL_HANDLE), allocateInfo() {}

  // TODO: have this inherit from a generic memory/buffer test so one of the
  // two doesn't have to be created here. For now, just have a massive SetUp()
  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    // completely arbitrary allocation for test purposes
    const size_t size = 16;

    VkExtent3D extent = {};
    extent.width = 42;
    extent.height = 42;
    extent.depth = 1;

    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.flags = VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = VK_FORMAT_R32G32B32_UINT;
    createInfo.extent = extent;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    createInfo.tiling = VK_IMAGE_TILING_LINEAR;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_GENERAL;

    vkCreateImage(device, &createInfo, nullptr, &image);

    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = size;

    vkAllocateMemory(device, &allocateInfo, nullptr, &deviceMemory);
  }

  virtual void TearDown() override {
    if (image) {
      vkDestroyImage(device, image, nullptr);
    }
    if (deviceMemory) {
      vkFreeMemory(device, deviceMemory, nullptr);
    }

    DeviceTest::TearDown();
  }

  VkImage image;
  VkDeviceMemory deviceMemory;
  VkMemoryAllocateInfo allocateInfo;
};

TEST_F(BindImageMemoryTest, DISABLED_Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkBindImageMemory(device, image, deviceMemory, 0));
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
