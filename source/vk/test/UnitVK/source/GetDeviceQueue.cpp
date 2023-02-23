// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkGetDeviceQueue

class GetDeviceQueue : public uvk::DeviceTest {
public:
  GetDeviceQueue() : queue(VK_NULL_HANDLE) {}
  VkQueue queue;
};

TEST_F(GetDeviceQueue, Default) {
  // since the device create info structs are initialized with = {}
  // in DeviceTest the index values are 0 by default
  vkGetDeviceQueue(device, 0, 0, &queue);

  ASSERT_NE(nullptr, queue);
}
