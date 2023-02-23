// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkResetDescriptorPool

class ResetDescriptorPool : public uvk::DescriptorPoolTest,
                            uvk::DescriptorSetLayoutTest {
 public:
  ResetDescriptorPool()
      : DescriptorSetLayoutTest(true), descriptorSet(VK_NULL_HANDLE) {}

  virtual void SetUp() {
    RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());
    RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
  }

  virtual void TearDown() {
    DescriptorSetLayoutTest::TearDown();
    DescriptorPoolTest::TearDown();
  }

  VkDescriptorSet descriptorSet;
};

TEST_F(ResetDescriptorPool, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkResetDescriptorPool(device, descriptorPool, 0));
}
