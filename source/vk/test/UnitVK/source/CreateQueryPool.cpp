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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateQueryPool

class CreateQueryPool : public uvk::DeviceTest {
 public:
  CreateQueryPool() : createInfo(), queryPool() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    createInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.pipelineStatistics =
        VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
  }

  virtual void TearDown() override {
    if (queryPool) {
      vkDestroyQueryPool(device, queryPool, nullptr);
    }

    DeviceTest::TearDown();
  }

  VkQueryPoolCreateInfo createInfo;
  VkQueryPool queryPool;
};

TEST_F(CreateQueryPool, DISABLED_Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateQueryPool(device, &createInfo, nullptr, &queryPool));
}

TEST_F(CreateQueryPool, DISABLED_DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateQueryPool(device, &createInfo,
                                     uvk::defaultAllocator(), &queryPool));
  vkDestroyQueryPool(device, queryPool, uvk::defaultAllocator());
  queryPool = VK_NULL_HANDLE;
}

TEST_F(CreateQueryPool, DISABLED_ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateQueryPool(device, &createInfo, uvk::nullAllocator(), &queryPool));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
