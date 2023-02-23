// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
