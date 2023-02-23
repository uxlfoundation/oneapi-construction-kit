// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkGetFenceStatus

class GetFenceStatusTest : public uvk::DeviceTest {
public:
  GetFenceStatusTest() : fence(VK_NULL_HANDLE), createInfo() {}

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  }

  VkFence fence;
  VkFenceCreateInfo createInfo;
};

TEST_F(GetFenceStatusTest, Default) {
  vkCreateFence(device, &createInfo, nullptr, &fence);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkGetFenceStatus(device, fence));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetFences(device, 1, &fence));
  ASSERT_EQ_RESULT(VK_NOT_READY, vkGetFenceStatus(device, fence));

  vkDestroyFence(device, fence, nullptr);
}

TEST_F(GetFenceStatusTest, DefaultAllocator) {
  vkCreateFence(device, &createInfo, uvk::defaultAllocator(), &fence);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkGetFenceStatus(device, fence));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetFences(device, 1, &fence));
  ASSERT_EQ_RESULT(VK_NOT_READY, vkGetFenceStatus(device, fence));

  vkDestroyFence(device, fence, uvk::defaultAllocator());
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
