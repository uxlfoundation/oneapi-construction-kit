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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkAllocateMemory

class AllocateMemory : public uvk::DeviceTest {
 public:
  AllocateMemory() : allocateInfo(), deviceMemory(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = 256;
  }

  virtual void TearDown() override {
    if (deviceMemory) {
      vkFreeMemory(device, deviceMemory, nullptr);
    }
    DeviceTest::TearDown();
  }

  VkMemoryAllocateInfo allocateInfo;
  VkDeviceMemory deviceMemory;
};

TEST_F(AllocateMemory, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateMemory(device, &allocateInfo, nullptr,
                                                &deviceMemory));
}

TEST_F(AllocateMemory, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkAllocateMemory(device, &allocateInfo,
                                    uvk::defaultAllocator(), &deviceMemory));

  vkFreeMemory(device, deviceMemory, uvk::defaultAllocator());
  deviceMemory = VK_NULL_HANDLE;
}

TEST_F(AllocateMemory, DefaultDeviceLocal) {
  uint32_t memoryTypeIndex;

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

  for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount;
       memoryTypeIndex++) {
    if (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT &
        memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags) {
      break;
    }
  }
  allocateInfo.memoryTypeIndex = memoryTypeIndex;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateMemory(device, &allocateInfo, nullptr,
                                                &deviceMemory));
}

TEST_F(AllocateMemory, DefaultAllocatorDeviceLocal) {
  uint32_t memoryTypeIndex;

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

  for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount;
       memoryTypeIndex++) {
    if (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT &
        memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags) {
      break;
    }
  }
  allocateInfo.memoryTypeIndex = memoryTypeIndex;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkAllocateMemory(device, &allocateInfo,
                                    uvk::defaultAllocator(), &deviceMemory));

  vkFreeMemory(device, deviceMemory, uvk::defaultAllocator());
  deviceMemory = VK_NULL_HANDLE;
}

TEST_F(AllocateMemory, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(VK_ERROR_OUT_OF_HOST_MEMORY,
                   vkAllocateMemory(device, &allocateInfo, uvk::nullAllocator(),
                                    &deviceMemory));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
//
// VK_ERROR_TOO_MANY_OBJECTS
// Is a possible return from this function, but is untestable because creating
// allocations up to the limit defined in physicalDeviceProperties is simply
// unrealistic (this number can be as high as ~500,000)
