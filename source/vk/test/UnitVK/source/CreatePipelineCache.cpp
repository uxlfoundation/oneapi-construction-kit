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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreatePipelineCache

class CreatePipelineCache : public uvk::PipelineLayoutTest {
 public:
  CreatePipelineCache()
      : pipelineCacheCreateInfo(), pipelineCache(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    pipelineCacheCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheCreateInfo.initialDataSize = 0;
    pipelineCacheCreateInfo.pInitialData = nullptr;
  }

  virtual void TearDown() override {
    if (pipelineCache) {
      vkDestroyPipelineCache(device, pipelineCache, nullptr);
    }

    if (pipelineLayout) {
      PipelineLayoutTest::TearDown();
    } else {
      DeviceTest::TearDown();
    }
  }

  VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
  VkPipelineCache pipelineCache;
};

TEST_F(CreatePipelineCache, Default) {
  RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
                                         nullptr, &pipelineCache));
}

TEST_F(CreatePipelineCache, DefaultInitialMemory) {
  RETURN_ON_FATAL_FAILURE(PipelineLayoutTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
                                         nullptr, &pipelineCache));

  const uvk::ShaderCode shaderCode = uvk::getShader(uvk::Shader::nop);

  VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
  shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderModuleCreateInfo.codeSize = shaderCode.size;
  shaderModuleCreateInfo.pCode =
      reinterpret_cast<const uint32_t *>(shaderCode.code);

  VkShaderModule shaderModule;
  ASSERT_EQ_RESULT(
      VK_SUCCESS, vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr,
                                       &shaderModule));

  VkPipelineShaderStageCreateInfo stage = {};
  stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stage.module = shaderModule;
  stage.pName = "main";
  stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;

  VkComputePipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = pipelineLayout;
  pipelineCreateInfo.stage = stage;

  VkPipeline pipeline;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateComputePipelines(device, pipelineCache,
                                                        1, &pipelineCreateInfo,
                                                        nullptr, &pipeline));

  vkDestroyShaderModule(device, shaderModule, nullptr);

  size_t dataSize;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkGetPipelineCacheData(device, pipelineCache,
                                                      &dataSize, nullptr));

  std::vector<char> cacheData(dataSize);

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkGetPipelineCacheData(device, pipelineCache, &dataSize,
                                          cacheData.data()));

  VkPipelineCache pipelineCacheInitialData;

  pipelineCacheCreateInfo.initialDataSize = cacheData.size();
  pipelineCacheCreateInfo.pInitialData = cacheData.data();

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
                                         nullptr, &pipelineCacheInitialData));

  vkDestroyPipeline(device, pipeline, nullptr);

  vkDestroyPipelineCache(device, pipelineCacheInitialData, nullptr);
}

TEST_F(CreatePipelineCache, ErrorOutOfHostMemory) {
  RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

  ASSERT_EQ_RESULT(VK_ERROR_OUT_OF_HOST_MEMORY,
                   vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
                                         uvk::nullAllocator(), &pipelineCache));
}
