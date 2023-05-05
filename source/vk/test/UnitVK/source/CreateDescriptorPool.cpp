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
