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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkGetPhysicalDeviceMemoryProperties

class GetPhysicalDeviceMemoryProperties : public uvk::PhysicalDeviceTest {
 public:
  GetPhysicalDeviceMemoryProperties() {}
};

TEST_F(GetPhysicalDeviceMemoryProperties, DefaultDeviceLocalHeap) {
  VkPhysicalDeviceMemoryProperties properties = {};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
  bool deviceLocalHeap = false;

  for (uint32_t heapIndex = 0; heapIndex < properties.memoryHeapCount;
       heapIndex++) {
    deviceLocalHeap |= (VK_MEMORY_HEAP_DEVICE_LOCAL_BIT &
                        properties.memoryHeaps[heapIndex].flags);
  }

  ASSERT_TRUE(deviceLocalHeap);
}

TEST_F(GetPhysicalDeviceMemoryProperties, DefaultHostVisibleType) {
  VkPhysicalDeviceMemoryProperties properties = {};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
  bool hostVisibleType = false;

  for (uint32_t typeIndex = 0; typeIndex < properties.memoryTypeCount;
       typeIndex++) {
    hostVisibleType |= (properties.memoryTypes[typeIndex].propertyFlags &
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  }

  ASSERT_TRUE(hostVisibleType);
}

TEST_F(GetPhysicalDeviceMemoryProperties, GetPhysicalDeviceMemoryProperties2) {
  if (!isInstanceExtensionEnabled(
          std::string("VK_KHR_get_physical_device_properties2"))) {
    GTEST_SKIP();
  }
  VkPhysicalDeviceMemoryProperties2 properties2 = {};

  // assuming the above both passed we should find the same doing it through
  // the extension
  vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &properties2);

  bool propertyFound = false;
  for (uint32_t heapIndex = 0;
       heapIndex < properties2.memoryProperties.memoryHeapCount; heapIndex++) {
    propertyFound |=
        (VK_MEMORY_HEAP_DEVICE_LOCAL_BIT &
         properties2.memoryProperties.memoryHeaps[heapIndex].flags);
  }

  ASSERT_TRUE(propertyFound);
  propertyFound = false;

  for (uint32_t typeIndex = 0;
       typeIndex < properties2.memoryProperties.memoryTypeCount; typeIndex++) {
    propertyFound |=
        (properties2.memoryProperties.memoryTypes[typeIndex].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  }

  ASSERT_TRUE(propertyFound);
}
