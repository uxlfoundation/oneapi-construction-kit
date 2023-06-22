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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateDescriptorSetLayout

class CreateDescriptorSetLayout : public uvk::DeviceTest {
 public:
  CreateDescriptorSetLayout()
      : createInfo(), descriptorSetLayout(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // create some bindings to test functionality
    bindings = {{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2,
                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, nullptr},
                {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1,
                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, nullptr}};
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();
  }

  virtual void TearDown() override {
    if (descriptorSetLayout) {
      vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    }
    DeviceTest::TearDown();
  }

  std::vector<VkDescriptorSetLayoutBinding> bindings;
  VkDescriptorSetLayoutCreateInfo createInfo;
  VkDescriptorSetLayout descriptorSetLayout;
};

TEST_F(CreateDescriptorSetLayout, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateDescriptorSetLayout(device, &createInfo, nullptr,
                                               &descriptorSetLayout));
}

TEST_F(CreateDescriptorSetLayout, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateDescriptorSetLayout(
                                   device, &createInfo, uvk::defaultAllocator(),
                                   &descriptorSetLayout));
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout,
                               uvk::defaultAllocator());
  descriptorSetLayout = VK_NULL_HANDLE;
}

TEST_F(CreateDescriptorSetLayout, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateDescriptorSetLayout(device, &createInfo, uvk::nullAllocator(),
                                  &descriptorSetLayout));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
