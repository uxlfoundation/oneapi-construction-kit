// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkAllocateDescriptorSets

class AllocateDescriptorSets : public uvk::DescriptorPoolTest {
public:
  AllocateDescriptorSets()
      : descriptorSetLayout(VK_NULL_HANDLE), allocInfo(),
        descriptorSet(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBinding.binding = 0;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = 1;
    layoutCreateInfo.pBindings = &layoutBinding;

    vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr,
                                &descriptorSetLayout);

    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    allocInfo.descriptorSetCount = 1;
  }

  virtual void TearDown() override {
    if (descriptorSet) {
      vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
    }
    if (descriptorSetLayout) {
      vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    }
    DescriptorPoolTest::TearDown();
  }

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSetAllocateInfo allocInfo;
  VkDescriptorSet descriptorSet;
};

TEST_F(AllocateDescriptorSets, Default) {
  ASSERT_EQ_RESULT(
      VK_SUCCESS, vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
}

TEST_F(AllocateDescriptorSets, ErrorOutOfHostMemory) {
  VkDescriptorPool nullPool;

  VkDescriptorPoolSize size = {};
  size.descriptorCount = 1;
  size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
  descriptorPoolCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.maxSets = 1;
  descriptorPoolCreateInfo.poolSizeCount = 1;
  descriptorPoolCreateInfo.pPoolSizes = &size;

  bool used = false;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateDescriptorPool(
                                   device, &descriptorPoolCreateInfo,
                                   uvk::oneUseAllocator(&used), &nullPool));

  allocInfo.descriptorPool = nullPool;

  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

  vkDestroyDescriptorPool(device, nullPool, uvk::oneUseAllocator(&used));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
