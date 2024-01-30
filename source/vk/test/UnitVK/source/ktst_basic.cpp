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

#include "kts/reference_functions.h"
#include "kts_vk.h"

using ktst_sgemm = kts::uvk::GenericKernelTest<uvk::Shader::kts_sgemm>;
TEST_F(ktst_sgemm, BasicCorrectnessTest) {
  kts::Reference1D<glsl::floatTy> refIn = kts::Ref_Float;
  kts::Reference1D<glsl::floatTy> refOut = [](size_t x) {
    switch (x / 4) {
      case 0:
        return 224 + 24 * (x % 4);
      case 1:
        return 608 + 88 * (x % 4);
      case 2:
        return 992 + 152 * (x % 4);
      default:
        return 1376 + 216 * (x % 4);
    }
  };
  AddPrimitive((glsl::uintTy)4);
  AddPrimitive((glsl::uintTy)4);
  AddPrimitive((glsl::uintTy)4);
  AddInputBuffer(16, refIn);
  AddInputBuffer(16, refIn);
  AddOutputBuffer(16, refOut);
  uint32_t global[3] = {2, 2, 1};
  RunGeneric(global);
}

using ktst_image = kts::uvk::GenericKernelTest<uvk::Shader::kts_image>;
TEST_F(ktst_image, DISABLED_BasicCorrectnessTest) {
  kts::Reference1D<glsl::vec4Ty> ref =
      kts::BuildVec4Reference1D<glsl::vec4Ty, glsl::floatTy>(kts::Ref_Float);
  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.imageType = VK_IMAGE_TYPE_2D;
  info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  info.extent = {4, 4, 1};
  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  info.usage = VK_IMAGE_USAGE_STORAGE_BIT;
  info.flags = 0;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkImageViewCreateInfo view = {};
  view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view.image = VK_NULL_HANDLE;
  view.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  view.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                     VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  AddInputImage(info, view, VK_IMAGE_LAYOUT_GENERAL, 16, ref);
  AddOutputBuffer(16, ref);
  uint32_t global[3] = {4, 4, 1};
  RunGeneric(global);
}

using ktst_sampler = kts::uvk::GenericKernelTest<uvk::Shader::kts_sampler>;
TEST_F(ktst_sampler, DISABLED_BasicCorrectnessTest) {
  kts::Reference1D<glsl::vec4Ty> ref =
      kts::BuildVec4Reference1D<glsl::vec4Ty, glsl::floatTy>(kts::Ref_Float);

  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.imageType = VK_IMAGE_TYPE_2D;
  info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  info.extent = {4, 4, 1};
  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
  info.flags = 0;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkImageViewCreateInfo view = {};
  view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view.image = VK_NULL_HANDLE;
  view.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  view.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                     VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  VkSamplerCreateInfo sampler = {};
  sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler.magFilter = VK_FILTER_NEAREST;
  sampler.minFilter = VK_FILTER_NEAREST;
  sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sampler.mipLodBias = 0;
  sampler.anisotropyEnable = VK_FALSE;
  sampler.minLod = 0;
  sampler.maxLod = 0;
  sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  sampler.unnormalizedCoordinates = VK_TRUE;
  sampler.compareEnable = VK_FALSE;

  AddSampler(info, view, VK_IMAGE_LAYOUT_GENERAL, sampler, 16, ref);
  AddOutputBuffer(16, ref);
  uint32_t global[3] = {4, 4, 1};
  RunGeneric(global);
}

using ktst_sgemm_custom =
    kts::uvk::GenericKernelTest<uvk::Shader::kts_sgemm_spec>;
TEST_F(ktst_sgemm_custom, BasicCorrectnessTest) {
  kts::Reference1D<glsl::floatTy> refIn = kts::Ref_Float;
  kts::Reference1D<glsl::floatTy> refOut = [](size_t x) {
    switch (x / 4) {
      case 0:
        return 224 + 24 * (x % 4);
      case 1:
        return 608 + 88 * (x % 4);
      case 2:
        return 992 + 152 * (x % 4);
      default:
        return 1376 + 216 * (x % 4);
    }
  };

  AddInputBuffer(16, refIn);
  AddInputBuffer(16, refIn);
  AddOutputBuffer(16, refOut);
  setUpShaderModule();
  VkShaderModule shaderModule = getShaderModule().value();
  VkPipelineLayout pipelineLayout = getPipelineLayout().value();

  glsl::uintTy prims[3] = {4, 4, 4};

  const VkSpecializationMapEntry entries[] =
      // id,  offset,                size
      {{0, 0, sizeof(glsl::uintTy)},
       {1, sizeof(glsl::uintTy), sizeof(glsl::uintTy)},
       {2, 2 * sizeof(glsl::uintTy), sizeof(glsl::uintTy)}};

  const VkSpecializationInfo spec_info = {
      3,                         // mapEntryCount
      entries,                   // pMapEntries
      3 * sizeof(glsl::uintTy),  // dataSize
      prims                      // pData
  };

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
  shaderStageCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.module = shaderModule;
  shaderStageCreateInfo.pName = "main";
  shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageCreateInfo.pSpecializationInfo = &spec_info;

  VkComputePipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = pipelineLayout;
  pipelineCreateInfo.stage = shaderStageCreateInfo;

  VkPipeline pipeline_;
  vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                           nullptr, &pipeline_);
  providePipeline(pipeline_);

  uint32_t global[3] = {2, 2, 1};
  RunGeneric(global);

  AddPrimitive((glsl::uintTy)4);
  AddPrimitive((glsl::uintTy)4);
  AddPrimitive((glsl::uintTy)4);
  AddInputBuffer(16, refIn);
  AddInputBuffer(16, refIn);
  AddOutputBuffer(16, refOut);

  const uvk::ShaderCode shaderCode = uvk::getShader(uvk::Shader::kts_sgemm);

  VkShaderModuleCreateInfo shaderCreateInfo = {};
  shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderCreateInfo.pCode = reinterpret_cast<const uint32_t *>(shaderCode.code);
  shaderCreateInfo.codeSize = shaderCode.size;

  VkShaderModule shaderModule_;
  vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule_);
  provideShaderModule(shaderModule_);
  RunGeneric(global);
}
