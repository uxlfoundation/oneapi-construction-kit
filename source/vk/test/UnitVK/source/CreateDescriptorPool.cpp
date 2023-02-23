// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateDescriptorPool

class CreateDescriptorPool : public uvk::DeviceTest {
public:
  CreateDescriptorPool()
      : poolSize(), createInfo(), descriptorPool(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    poolSize.descriptorCount = 1;
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.maxSets = 1;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &poolSize;
  }

  virtual void TearDown() override {
    if (descriptorPool) {
      vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }

    DeviceTest::TearDown();
  }

  VkDescriptorPoolSize poolSize;
  VkDescriptorPoolCreateInfo createInfo;
  VkDescriptorPool descriptorPool;
};

TEST_F(CreateDescriptorPool, Default) {
  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool));
}

TEST_F(CreateDescriptorPool, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateDescriptorPool(device, &createInfo,
                                                      uvk::defaultAllocator(),
                                                      &descriptorPool));
  vkDestroyDescriptorPool(device, descriptorPool, uvk::defaultAllocator());
  descriptorPool = VK_NULL_HANDLE;
}

TEST_F(CreateDescriptorPool, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(VK_ERROR_OUT_OF_HOST_MEMORY,
                   vkCreateDescriptorPool(device, &createInfo,
                                          uvk::nullAllocator(),
                                          &descriptorPool));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
