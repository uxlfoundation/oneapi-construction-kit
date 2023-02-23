

// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateFence

class CreateFence : public uvk::DeviceTest {
public:
  CreateFence() : fence(VK_NULL_HANDLE), createInfo() {}

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  }

  void TearDown() override {
    if (fence) {
      vkDestroyFence(device, fence, nullptr);
    }
    DeviceTest::TearDown();
  }

  VkFence fence;
  VkFenceCreateInfo createInfo;
};

TEST_F(CreateFence, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateFence(device, &createInfo, nullptr, &fence));
}

TEST_F(CreateFence, DefaultFlagsSignaled) {
  createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateFence(device, &createInfo, nullptr, &fence));
  ASSERT_EQ_RESULT(vkGetFenceStatus(device, fence), VK_SUCCESS);
}

TEST_F(CreateFence, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateFence(device, &createInfo,
                                             uvk::defaultAllocator(), &fence));

  vkDestroyFence(device, fence, uvk::defaultAllocator());
  fence = VK_NULL_HANDLE;
}

TEST_F(CreateFence, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateFence(device, &createInfo, uvk::nullAllocator(), &fence));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable due to the fact
// that we can't currently access device memory  allocators to mess with.
