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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkGetPhysicalDeviceQueueFamilyProperties

class GetPhysicalDeviceQueueFamilyProperties : public uvk::PhysicalDeviceTest {
 public:
  GetPhysicalDeviceQueueFamilyProperties() {}
  std::vector<VkQueueFamilyProperties> queueFamilyProperties;
};

TEST_F(GetPhysicalDeviceQueueFamilyProperties, Default) {
  uint32_t propertyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount,
                                           nullptr);
  ASSERT_TRUE(propertyCount > 0);
  queueFamilyProperties.resize(propertyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount,
                                           queueFamilyProperties.data());
  for (const auto &p : queueFamilyProperties) {
    ASSERT_TRUE(p.queueCount > 0);
  }
}

TEST_F(GetPhysicalDeviceQueueFamilyProperties,
       GetPhysicalDeviceQueueFamilyProperties2) {
  if (!isInstanceExtensionEnabled(
          std::string("VK_KHR_get_physical_device_properties2"))) {
    GTEST_SKIP();
  }
  uint32_t propertyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &propertyCount,
                                            nullptr);
  ASSERT_TRUE(propertyCount > 0);
  std::vector<VkQueueFamilyProperties2> queueFamilyProperties2(propertyCount);
  vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &propertyCount,
                                            queueFamilyProperties2.data());
  for (const auto &p : queueFamilyProperties2) {
    ASSERT_TRUE(p.queueFamilyProperties.queueCount > 0);
  }
}
