// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
  for (VkQueueFamilyProperties p : queueFamilyProperties) {
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
  for (VkQueueFamilyProperties2 p : queueFamilyProperties2) {
    ASSERT_TRUE(p.queueFamilyProperties.queueCount > 0);
  }
}
