// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkGetPipelineCacheData

class GetPipelineCacheData : public uvk::PipelineLayoutTest {
 public:
  GetPipelineCacheData() : pipelineCacheCreateInfo({}) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(PipelineLayoutTest::SetUp());

    pipelineCacheCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
                                           nullptr, &pipelineCache));

    uvk::ShaderCode shaderCode = uvk::getShader(uvk::Shader::nop);

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderCode.size;
    shaderModuleCreateInfo.pCode =
        reinterpret_cast<const uint32_t*>(shaderCode.code);

    VkShaderModule shaderModule;

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreateShaderModule(device, &shaderModuleCreateInfo,
                                          nullptr, &shaderModule));

    VkPipelineShaderStageCreateInfo stage = {};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = shaderModule;
    stage.pName = "main";

    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.stage = stage;

    VkPipeline pipeline;

    ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateComputePipelines(
                                     device, pipelineCache, 1,
                                     &pipelineCreateInfo, nullptr, &pipeline));
    vkDestroyPipeline(device, pipeline, nullptr);

    vkDestroyShaderModule(device, shaderModule, nullptr);
  }

  virtual void TearDown() override {
    vkDestroyPipelineCache(device, pipelineCache, nullptr);
    PipelineLayoutTest::TearDown();
  }

  VkPipelineCache pipelineCache;
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
};

TEST_F(GetPipelineCacheData, Default) {
  size_t dataSize;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkGetPipelineCacheData(device, pipelineCache,
                                                      &dataSize, nullptr));

  std::vector<char> data(dataSize);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkGetPipelineCacheData(device, pipelineCache,
                                                      &dataSize, data.data()));
}

TEST_F(GetPipelineCacheData, ErrorIncomplete) {
  size_t dataSize;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkGetPipelineCacheData(device, pipelineCache,
                                                      &dataSize, nullptr));

  dataSize--;
  std::vector<char> data(dataSize);

  ASSERT_EQ_RESULT(
      VK_INCOMPLETE,
      vkGetPipelineCacheData(device, pipelineCache, &dataSize, data.data()));
}

TEST_F(GetPipelineCacheData, SaveDataBetweenRuns) {
  size_t dataSize;

  ASSERT_EQ_RESULT(VK_SUCCESS, vkGetPipelineCacheData(device, pipelineCache,
                                                      &dataSize, nullptr));

  std::vector<char> data(dataSize);

  ASSERT_EQ_RESULT(VK_SUCCESS, vkGetPipelineCacheData(device, pipelineCache,
                                                      &dataSize, data.data()));

  // destroy everything down to the instance and then re-create everything again
  // to make sure we can save and use cache data between runs without crashing
  GetPipelineCacheData::TearDown();
  GetPipelineCacheData::pipelineCacheCreateInfo.initialDataSize = dataSize;
  GetPipelineCacheData::pipelineCacheCreateInfo.pInitialData = data.data();
  RETURN_ON_FATAL_FAILURE(GetPipelineCacheData::SetUp());
}
