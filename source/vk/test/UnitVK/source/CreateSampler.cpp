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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateSampler

class CreateSamplerTest : public uvk::DeviceTest {
 public:
  CreateSamplerTest() : createInfo(), sampler(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  }

  virtual void TearDown() override {
    if (sampler) {
      vkDestroySampler(device, sampler, nullptr);
    }
    DeviceTest::TearDown();
  }

  VkSamplerCreateInfo createInfo;
  VkSampler sampler;
};

TEST_F(CreateSamplerTest, DISABLED_Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateSampler(device, &createInfo, nullptr, &sampler));
}

TEST_F(CreateSamplerTest, DISABLED_DefaultAllocator) {
  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreateSampler(device, &createInfo, uvk::defaultAllocator(), &sampler));
  vkDestroySampler(device, sampler, uvk::defaultAllocator());
  sampler = VK_NULL_HANDLE;
}

TEST_F(CreateSamplerTest, DISABLED_ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateSampler(device, &createInfo, uvk::nullAllocator(), &sampler));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
//
// TODO: Find a way to test VK_ERROR_TOO_MANY_OBJECTS
