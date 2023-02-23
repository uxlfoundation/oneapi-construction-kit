// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
