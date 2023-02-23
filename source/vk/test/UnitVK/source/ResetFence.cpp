// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkResetFences

class ResetFences : public uvk::DeviceTest {
public:
  ResetFences() : fence(VK_NULL_HANDLE) {}

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    // create fence in the signaled state
    VkFenceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(device, &createInfo, nullptr, &fence);
  }

  void TearDown() override {
    vkDestroyFence(device, fence, nullptr);
    DeviceTest::TearDown();
  }

  VkFence fence;
};

TEST_F(ResetFences, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetFences(device, 1, &fence));

  // check that the fence was reset to unsignaled state
  ASSERT_EQ_RESULT(VK_NOT_READY, vkGetFenceStatus(device, fence));
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
