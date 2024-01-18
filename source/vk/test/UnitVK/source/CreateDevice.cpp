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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateDevice

class CreateDevice : public uvk::PhysicalDeviceTest {
 public:
  CreateDevice()
      : deviceCreateInfo(), queueCreateInfo(), device(VK_NULL_HANDLE) {}
  virtual void SetUp() {
    RETURN_ON_FATAL_FAILURE(PhysicalDeviceTest::SetUp());

    const float queuePriority = 1;

    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
  }

  virtual void TearDown() {
    if (device) {
      vkDestroyDevice(device, nullptr);
    }

    PhysicalDeviceTest::TearDown();
  }

  VkDeviceCreateInfo deviceCreateInfo;
  VkDeviceQueueCreateInfo queueCreateInfo;
  VkDevice device;
};

TEST_F(CreateDevice, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateDevice(physicalDevice, &deviceCreateInfo,
                                              nullptr, &device));
}

TEST_F(CreateDevice, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateDevice(physicalDevice, &deviceCreateInfo,
                                  uvk::defaultAllocator(), &device));
  vkDestroyDevice(device, uvk::defaultAllocator());
  device = VK_NULL_HANDLE;
}

TEST_F(CreateDevice, DISABLED_DefaultLayer) {
  // this is a deprecated feature
  const char *layerName = "VK_LAYER_LUNARG_core_validation";

  // since enabled layers on device and instance must match, the instance
  // needs to be reinitialized with a layer enabled
  instanceCreateInfo.enabledLayerCount = 1;
  instanceCreateInfo.ppEnabledLayerNames = &layerName;

  RETURN_ON_FATAL_FAILURE(PhysicalDeviceTest::SetUp());

  deviceCreateInfo.enabledLayerCount = 1;
  deviceCreateInfo.ppEnabledLayerNames = &layerName;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateDevice(physicalDevice, &deviceCreateInfo,
                                              nullptr, &device));
}

TEST_F(CreateDevice, DefaultExtension) {
  uint32_t propCount = 0;
  std::string extensionLayer = "";

  for (const char *layer : enabledLayerNames) {
    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkEnumerateDeviceExtensionProperties(physicalDevice, layer,
                                                          &propCount, nullptr));

    if (0 < propCount) {
      // save the layer that reported the extension
      extensionLayer = layer;
      break;
    }
  }

  if (0 < propCount) {
    std::vector<VkExtensionProperties> properties(propCount);

    ASSERT_EQ_RESULT(VK_SUCCESS, vkEnumerateDeviceExtensionProperties(
                                     physicalDevice, extensionLayer.data(),
                                     &propCount, properties.data()));

    const char *const extensionName = properties[0].extensionName;

    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = &extensionName;

    ASSERT_EQ_RESULT(
        VK_SUCCESS,
        vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
  }
}

TEST_F(CreateDevice, DefaultFeature) {
  const VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
  deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateDevice(physicalDevice, &deviceCreateInfo,
                                              nullptr, &device));
}

TEST_F(CreateDevice, ErrorOutOfHostMemory) {
  // create and use an allocator which can only return nullptr to force
  // the out of host memory error
  ASSERT_EQ_RESULT(VK_ERROR_OUT_OF_HOST_MEMORY,
                   vkCreateDevice(physicalDevice, &deviceCreateInfo,
                                  uvk::nullAllocator(), &device));
}

// disabled because device layers are deprecated so the loader completely
// ignores any enabled layers passed here, extant or not
TEST_F(CreateDevice, DISABLED_ErrorLayerNotPresent) {
  // try to enable a non-existant layer
  const char *dummyLayerName = "not really a layer name";

  deviceCreateInfo.enabledLayerCount = 1;
  deviceCreateInfo.ppEnabledLayerNames = &dummyLayerName;

  ASSERT_EQ_RESULT(
      VK_ERROR_LAYER_NOT_PRESENT,
      vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
}

TEST_F(CreateDevice, ErrorExtensionNotPresent) {
  // try to enable a non-existant extension
  const char *dummyExtensionName = "not really an extension name";

  deviceCreateInfo.enabledExtensionCount = 1;
  deviceCreateInfo.ppEnabledExtensionNames = &dummyExtensionName;

  ASSERT_EQ_RESULT(
      VK_ERROR_EXTENSION_NOT_PRESENT,
      vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
//
// VK_ERROR_DEVICE_LOST
// Is a possible return from this function, but is untestable
// as the conditions it returns under cannot be safely replicated
//
// VK_ERROR_FEATURE_NOT_PRESENT
// Is a possible return from this function, but is untestable
// becasue it relies on the hardware specs of the machine running
// it to generate
//
// VK_ERROR_TOO_MANY_OBJECTS
// Is a possible return from this function, but is untestable
// as it can only be triggered under certain device specific circumstances
