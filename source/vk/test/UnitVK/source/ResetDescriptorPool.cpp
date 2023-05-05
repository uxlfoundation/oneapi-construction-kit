// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
