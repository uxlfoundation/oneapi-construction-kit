// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateCommandPool

class CreateCommandPool : public uvk::DeviceTest {
public:
  CreateCommandPool() : createInfo(), commandPool() {}

  virtual void SetUp() {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  }

  virtual void TearDown() {
    if (commandPool) {
      vkDestroyCommandPool(device, commandPool, nullptr);
    }
    DeviceTest::TearDown();
  }

  VkCommandPoolCreateInfo createInfo;
  VkCommandPool commandPool;
};

TEST_F(CreateCommandPool, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateCommandPool(device, &createInfo, nullptr,
                                                   &commandPool));
}

TEST_F(CreateCommandPool, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateCommandPool(device, &createInfo,
                                       uvk::defaultAllocator(), &commandPool));
  vkDestroyCommandPool(device, commandPool, uvk::defaultAllocator());
  commandPool = VK_NULL_HANDLE;
}

TEST_F(CreateCommandPool, DefaultFlagsTransient) {
  // Create with the transient flag enabled
  createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateCommandPool(device, &createInfo, nullptr,
                                                   &commandPool));
}

TEST_F(CreateCommandPool, DefaultFlagsResetCommandBuffer) {
  // Create with the reset command buffer flag enabled
  createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateCommandPool(device, &createInfo, nullptr,
                                                   &commandPool));
}

TEST_F(CreateCommandPool, DefaultFlagsAll) {
  // Create with both flags enabled
  createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateCommandPool(device, &createInfo, nullptr,
                                                   &commandPool));
}

TEST_F(CreateCommandPool, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(VK_ERROR_OUT_OF_HOST_MEMORY,
                   vkCreateCommandPool(device, &createInfo,
                                       uvk::nullAllocator(), &commandPool));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
