// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkGetPhysicalDeviceProperties

class GetPhysicalDeviceProperties : public uvk::PhysicalDeviceTest {
public:
  GetPhysicalDeviceProperties() {}
};

TEST_F(GetPhysicalDeviceProperties, Default) {
  // since this function doesn't have a return code, zero initialize the struct
  // and check some members which shouldn't be zero aren't zero
  VkPhysicalDeviceProperties physicalDeviceProperties = {};
  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
  ASSERT_NE(physicalDeviceProperties.apiVersion, 0u);
  ASSERT_NE(physicalDeviceProperties.driverVersion, 0u);
  ASSERT_NE(physicalDeviceProperties.vendorID, 0u);
  ASSERT_NE(physicalDeviceProperties.deviceID, 0u);
}

TEST_F(GetPhysicalDeviceProperties, DefaultDeviceTypeValid) {
  // don't initialize to zero here because zero is a valid device type
  VkPhysicalDeviceProperties physicalDeviceProperties = {};
  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

  ASSERT_TRUE(
      physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ||
      physicalDeviceProperties.deviceType ==
          VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
      physicalDeviceProperties.deviceType ==
          VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
      physicalDeviceProperties.deviceType ==
          VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ||
      physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER);
}

TEST_F(GetPhysicalDeviceProperties, DefaultDeviceLimitsValid) {
  // the VkPhysicalDeviceLimits struct is pretty large so just check a few
  // members for sanity
  VkPhysicalDeviceProperties physicalDeviceProperties = {};
  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
  ASSERT_TRUE(physicalDeviceProperties.limits.maxImageDimension2D == 0 ||
              physicalDeviceProperties.limits.maxImageDimension2D >= 4096);
  ASSERT_TRUE(physicalDeviceProperties.limits.maxComputeWorkGroupInvocations >
              0);
  ASSERT_TRUE(physicalDeviceProperties.limits.maxMemoryAllocationCount > 0);
  ASSERT_TRUE(physicalDeviceProperties.limits.maxBoundDescriptorSets > 0);
}

TEST_F(GetPhysicalDeviceProperties, GetPhysicalDeviceProperties2) {
  if (!isInstanceExtensionEnabled(
          std::string("VK_KHR_get_physical_device_properties2"))) {
    GTEST_SKIP();
  }
  VkPhysicalDeviceProperties2 physicalDeviceProperties = {};
  physicalDeviceProperties.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

  vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);

  // check we're getting properties returned correctly
  ASSERT_NE(physicalDeviceProperties.properties.apiVersion, 0u);
  ASSERT_NE(physicalDeviceProperties.properties.driverVersion, 0u);
  ASSERT_NE(physicalDeviceProperties.properties.vendorID, 0u);
  ASSERT_NE(physicalDeviceProperties.properties.deviceID, 0u);
}
