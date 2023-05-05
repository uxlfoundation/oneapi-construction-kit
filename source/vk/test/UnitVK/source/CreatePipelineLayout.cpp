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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreatePipelineLayout

class CreatePipelineLayout : public uvk::DescriptorSetLayoutTest {
public:
 CreatePipelineLayout() : createInfo(), pipelineLayout(VK_NULL_HANDLE) {}

 virtual void SetUp() override {
   RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

   createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   createInfo.setLayoutCount = 1;
   createInfo.pSetLayouts = &descriptorSetLayout;
  }

  virtual void TearDown() override {
    if (pipelineLayout) {
      vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }

    DescriptorSetLayoutTest::TearDown();
  }

  VkPipelineLayoutCreateInfo createInfo;
  VkPipelineLayout pipelineLayout;
};

TEST_F(CreatePipelineLayout, Default) {
  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreatePipelineLayout(device, &createInfo, nullptr, &pipelineLayout));
}

TEST_F(CreatePipelineLayout, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreatePipelineLayout(device, &createInfo,
                                                      uvk::defaultAllocator(),
                                                      &pipelineLayout));
  vkDestroyPipelineLayout(device, pipelineLayout, uvk::defaultAllocator());
  pipelineLayout = VK_NULL_HANDLE;
}

TEST_F(CreatePipelineLayout, DefaultPushConstantRange) {
  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.offset = 0;
  pushConstantRange.size = 16;
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  createInfo.pPushConstantRanges = &pushConstantRange;
  createInfo.pushConstantRangeCount = 1;

  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreatePipelineLayout(device, &createInfo, nullptr, &pipelineLayout));
}

TEST_F(CreatePipelineLayout, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(VK_ERROR_OUT_OF_HOST_MEMORY,
                   vkCreatePipelineLayout(device, &createInfo,
                                          uvk::nullAllocator(),
                                          &pipelineLayout));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
