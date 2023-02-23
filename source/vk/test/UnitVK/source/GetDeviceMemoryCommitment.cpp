// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkGetDeviceMemoryCommitment

class GetDeviceMemoryCommitment : public uvk::DeviceMemoryTest {
 public:
  GetDeviceMemoryCommitment() {}
};

TEST_F(GetDeviceMemoryCommitment, DISABLED_Default) {
  // TODO: implement an actual test for this when (if?) we implement lazily
  // allocated memory
  VkDeviceSize size;
  vkGetDeviceMemoryCommitment(device, memory, &size);
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
