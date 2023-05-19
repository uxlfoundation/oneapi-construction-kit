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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkEnumeratePhysicalDevices

class EnumeratePhysicalDevices : public uvk::InstanceTest {
 public:
  EnumeratePhysicalDevices() {}

  std::vector<VkPhysicalDevice> physicalDevices;
};

TEST_F(EnumeratePhysicalDevices, Default) {
  uint32_t deviceCount = 0;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

  physicalDevices.resize(deviceCount);

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkEnumeratePhysicalDevices(instance, &deviceCount,
                                              physicalDevices.data()));
}

// VK_INCOMPLETE
// Is a possible return from this function, but is untestable as
// it can only be returned if there are multiple Vulkan compatible
// hardware devices in the machine running the test.
//
// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
//
// VK_ERROR_INITIALIZATION_FAILED
// Is a possible return from this function, but is untestable
// because it can't actually be generated using only api calls.
