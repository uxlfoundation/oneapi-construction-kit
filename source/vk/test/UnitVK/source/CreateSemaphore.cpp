// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateSemaphore

class CreateSemaphore : public uvk::DeviceTest {
public:
  CreateSemaphore() : createInfo(), semaphore(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  }

  virtual void TearDown() override {
    if (semaphore) {
      vkDestroySemaphore(device, semaphore, nullptr);
    }
    DeviceTest::TearDown();
  }

  VkSemaphoreCreateInfo createInfo;
  VkSemaphore semaphore;
};

TEST_F(CreateSemaphore, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateSemaphore(device, &createInfo, nullptr, &semaphore));
}

TEST_F(CreateSemaphore, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateSemaphore(device, &createInfo,
                                     uvk::defaultAllocator(), &semaphore));
  vkDestroySemaphore(device, semaphore, uvk::defaultAllocator());
  semaphore = VK_NULL_HANDLE;
}

TEST_F(CreateSemaphore, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateSemaphore(device, &createInfo, uvk::nullAllocator(), &semaphore));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable due to the fact
// that we can't currently access device memory  allocators to mess with.
