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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateComputePipelines

class CreateComputePipelines : public uvk::PipelineLayoutTest {
 public:
  CreateComputePipelines()
      : shaderCreateInfo(),
        shaderModule(VK_NULL_HANDLE),
        shaderStageCreateInfo(),
        pipelineCreateInfo(),
        pipeline(VK_NULL_HANDLE) {}

  virtual void SetUp() {
    RETURN_ON_FATAL_FAILURE(PipelineLayoutTest::SetUp());

    const uvk::ShaderCode shaderCode = uvk::getShader(uvk::Shader::nop);
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.pCode = (uint32_t *)shaderCode.code;
    shaderCreateInfo.codeSize = shaderCode.size;

    vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule);

    shaderStageCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
  }

  virtual void TearDown() {
    vkDestroyShaderModule(device, shaderModule, nullptr);
    if (pipeline) {
      vkDestroyPipeline(device, pipeline, nullptr);
    }
    PipelineLayoutTest::TearDown();
  }

  VkShaderModuleCreateInfo shaderCreateInfo;
  VkShaderModule shaderModule;
  VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
  VkComputePipelineCreateInfo pipelineCreateInfo;
  VkPipeline pipeline;
};

TEST_F(CreateComputePipelines, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateComputePipelines(device, VK_NULL_HANDLE,
                                                        1, &pipelineCreateInfo,
                                                        nullptr, &pipeline));
}

TEST_F(CreateComputePipelines, DefaultAllocator) {
  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                               uvk::defaultAllocator(), &pipeline));
  vkDestroyPipeline(device, pipeline, uvk::defaultAllocator());
  pipeline = VK_NULL_HANDLE;
}

TEST_F(CreateComputePipelines, DefaultDerivativeBaseHandle) {
  pipelineCreateInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateComputePipelines(device, VK_NULL_HANDLE,
                                                        1, &pipelineCreateInfo,
                                                        nullptr, &pipeline));

  VkComputePipelineCreateInfo derivativeCreateInfo = {};
  derivativeCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  derivativeCreateInfo.basePipelineHandle = pipeline;
  derivativeCreateInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
  derivativeCreateInfo.layout = pipelineLayout;
  derivativeCreateInfo.stage = shaderStageCreateInfo;

  VkPipeline derivedPipeline;

  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &derivativeCreateInfo,
                               nullptr, &derivedPipeline));

  vkDestroyPipeline(device, derivedPipeline, nullptr);
}

TEST_F(CreateComputePipelines, DefaultDerivativeBaseIndex) {
  pipelineCreateInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

  VkComputePipelineCreateInfo derivativeCreateInfo = {};
  derivativeCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  derivativeCreateInfo.basePipelineIndex = 0;
  derivativeCreateInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
  derivativeCreateInfo.layout = pipelineLayout;
  derivativeCreateInfo.stage = shaderStageCreateInfo;

  std::vector<VkComputePipelineCreateInfo> createInfos = {pipelineCreateInfo,
                                                          derivativeCreateInfo};
  std::vector<VkPipeline> pipelines(2);

  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreateComputePipelines(device, VK_NULL_HANDLE, 2, createInfos.data(),
                               nullptr, pipelines.data()));

  for (int pIndex = 0; pIndex < 2; pIndex++) {
    vkDestroyPipeline(device, pipelines[pIndex], nullptr);
  }
}

TEST_F(CreateComputePipelines, DefaultSpecializationInfo) {
  const uvk::ShaderCode shaderCode = uvk::getShader(uvk::Shader::spec_const);

  VkShaderModule specConstantSModule;

  VkShaderModuleCreateInfo sModuleCreateInf = {};
  sModuleCreateInf.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  sModuleCreateInf.pCode = reinterpret_cast<const uint32_t *>(shaderCode.code);
  sModuleCreateInf.codeSize = shaderCode.size;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateShaderModule(device, &sModuleCreateInf, nullptr,
                                        &specConstantSModule));

  uint32_t specData = 42;

  VkSpecializationMapEntry specMapEntry = {};
  specMapEntry.constantID = 0;
  specMapEntry.offset = 0;
  specMapEntry.size = sizeof(specData);

  VkSpecializationInfo specInfo = {};
  specInfo.dataSize = sizeof(specData);
  specInfo.mapEntryCount = 1;
  specInfo.pData = reinterpret_cast<void *>(&specData);
  specInfo.pMapEntries = &specMapEntry;

  VkPipelineShaderStageCreateInfo specConstantStage = {};
  specConstantStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  specConstantStage.pName = "main";
  specConstantStage.module = specConstantSModule;
  specConstantStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  specConstantStage.pSpecializationInfo = &specInfo;

  pipelineCreateInfo.stage = specConstantStage;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateComputePipelines(device, VK_NULL_HANDLE,
                                                        1, &pipelineCreateInfo,
                                                        nullptr, &pipeline));

  vkDestroyShaderModule(device, specConstantSModule, nullptr);
}

TEST_F(CreateComputePipelines, DefaultPipelineCache) {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

  VkPipelineCache pipelineCache;

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
                                         nullptr, &pipelineCache));

  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateComputePipelines(device, pipelineCache,
                                                        1, &pipelineCreateInfo,
                                                        nullptr, &pipeline));

  VkPipeline newPipeliine;

  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreateComputePipelines(device, pipelineCache, 1, &pipelineCreateInfo,
                               nullptr, &newPipeliine));

  vkDestroyPipeline(device, newPipeliine, nullptr);
  vkDestroyPipelineCache(device, pipelineCache, nullptr);
}

TEST_F(CreateComputePipelines, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                               uvk::nullAllocator(), &pipeline));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable due to the fact
// that we can't currently access device memory  allocators to mess with.
