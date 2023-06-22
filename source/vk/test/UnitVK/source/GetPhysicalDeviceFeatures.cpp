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

// https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#vkGetPhysicalDeviceFeatures

class GetPhysicalDeviceFeatures : public uvk::PhysicalDeviceTest {
 public:
  GetPhysicalDeviceFeatures() {}
};

// Macro to check that arg is a valid VkBool
#define EXPECT_VK_BOOL(arg) EXPECT_TRUE((arg) == VK_FALSE || (arg) == VK_TRUE);

// Macro to check that arg is VK_TRUE
#define EXPECT_VK_TRUE(arg) EXPECT_TRUE((arg) == VK_TRUE);

TEST_F(GetPhysicalDeviceFeatures, Default) {
  VkPhysicalDeviceFeatures features;

  vkGetPhysicalDeviceFeatures(physicalDevice, &features);

  // Expect all graphics things to be false
  EXPECT_FALSE(features.alphaToOne);
  EXPECT_FALSE(features.depthBiasClamp);
  EXPECT_FALSE(features.depthBiasClamp);
  EXPECT_FALSE(features.depthBounds);
  EXPECT_FALSE(features.depthClamp);
  EXPECT_FALSE(features.drawIndirectFirstInstance);
  EXPECT_FALSE(features.dualSrcBlend);
  EXPECT_FALSE(features.fillModeNonSolid);
  EXPECT_FALSE(features.fragmentStoresAndAtomics);
  EXPECT_FALSE(features.fullDrawIndexUint32);
  EXPECT_FALSE(features.geometryShader);
  EXPECT_FALSE(features.independentBlend);
  EXPECT_FALSE(features.largePoints);
  EXPECT_FALSE(features.logicOp);
  EXPECT_FALSE(features.multiDrawIndirect);
  EXPECT_FALSE(features.multiViewport);
  EXPECT_FALSE(features.occlusionQueryPrecise);
  EXPECT_FALSE(features.samplerAnisotropy);
  EXPECT_FALSE(features.sampleRateShading);
  EXPECT_FALSE(features.shaderClipDistance);
  EXPECT_FALSE(features.shaderCullDistance);
  EXPECT_FALSE(features.shaderTessellationAndGeometryPointSize);
  EXPECT_FALSE(features.tessellationShader);
  EXPECT_FALSE(features.textureCompressionASTC_LDR);
  EXPECT_FALSE(features.textureCompressionBC);
  EXPECT_FALSE(features.textureCompressionETC2);
  EXPECT_FALSE(features.variableMultisampleRate);
  EXPECT_FALSE(features.vertexPipelineStoresAndAtomics);
  EXPECT_FALSE(features.wideLines);
  EXPECT_FALSE(features.imageCubeArray);
  EXPECT_FALSE(features.shaderResourceMinLod);

  // Expect device specific things to be either true or false
  EXPECT_VK_BOOL(features.inheritedQueries);
  EXPECT_VK_BOOL(features.pipelineStatisticsQuery);
  EXPECT_VK_BOOL(features.robustBufferAccess);
  EXPECT_VK_BOOL(features.shaderImageGatherExtended);
  EXPECT_VK_BOOL(features.shaderSampledImageArrayDynamicIndexing);
  EXPECT_VK_BOOL(features.shaderStorageBufferArrayDynamicIndexing);
  EXPECT_VK_BOOL(features.shaderStorageImageArrayDynamicIndexing);
  EXPECT_VK_BOOL(features.shaderStorageImageExtendedFormats);
  EXPECT_VK_BOOL(features.shaderStorageImageMultisample);
  EXPECT_VK_BOOL(features.shaderStorageImageReadWithoutFormat);
  EXPECT_VK_BOOL(features.shaderStorageImageWriteWithoutFormat);
  EXPECT_VK_BOOL(features.shaderUniformBufferArrayDynamicIndexing);
  EXPECT_VK_BOOL(features.sparseBinding);
  EXPECT_VK_BOOL(features.sparseResidency16Samples);
  EXPECT_VK_BOOL(features.sparseResidency8Samples);
  EXPECT_VK_BOOL(features.sparseResidency4Samples);
  EXPECT_VK_BOOL(features.sparseResidency2Samples);
  EXPECT_VK_BOOL(features.sparseResidencyAliased);
  EXPECT_VK_BOOL(features.sparseResidencyBuffer);
  EXPECT_VK_BOOL(features.sparseResidencyImage2D);
  EXPECT_VK_BOOL(features.sparseResidencyImage3D);
  EXPECT_VK_BOOL(features.shaderFloat64);
  EXPECT_VK_BOOL(features.shaderInt16);
  EXPECT_VK_BOOL(features.shaderInt64);
}

TEST_F(GetPhysicalDeviceFeatures, GetPhysicalDeviceFeatures2) {
  if (!isInstanceExtensionEnabled(
          std::string("VK_KHR_get_physical_device_properties2"))) {
    GTEST_SKIP();
  }

  VkPhysicalDeviceFeatures2 features = {};
  vkGetPhysicalDeviceFeatures2(physicalDevice, &features);

  // Expect all graphics things to be false
  EXPECT_FALSE(features.features.alphaToOne);
  EXPECT_FALSE(features.features.depthBiasClamp);
  EXPECT_FALSE(features.features.depthBiasClamp);
  EXPECT_FALSE(features.features.depthBounds);
  EXPECT_FALSE(features.features.depthClamp);
  EXPECT_FALSE(features.features.drawIndirectFirstInstance);
  EXPECT_FALSE(features.features.dualSrcBlend);
  EXPECT_FALSE(features.features.fillModeNonSolid);
  EXPECT_FALSE(features.features.fragmentStoresAndAtomics);
  EXPECT_FALSE(features.features.fullDrawIndexUint32);
  EXPECT_FALSE(features.features.geometryShader);
  EXPECT_FALSE(features.features.independentBlend);
  EXPECT_FALSE(features.features.largePoints);
  EXPECT_FALSE(features.features.logicOp);
  EXPECT_FALSE(features.features.multiDrawIndirect);
  EXPECT_FALSE(features.features.multiViewport);
  EXPECT_FALSE(features.features.occlusionQueryPrecise);
  EXPECT_FALSE(features.features.samplerAnisotropy);
  EXPECT_FALSE(features.features.sampleRateShading);
  EXPECT_FALSE(features.features.shaderClipDistance);
  EXPECT_FALSE(features.features.shaderCullDistance);
  EXPECT_FALSE(features.features.shaderTessellationAndGeometryPointSize);
  EXPECT_FALSE(features.features.tessellationShader);
  EXPECT_FALSE(features.features.textureCompressionASTC_LDR);
  EXPECT_FALSE(features.features.textureCompressionBC);
  EXPECT_FALSE(features.features.textureCompressionETC2);
  EXPECT_FALSE(features.features.variableMultisampleRate);
  EXPECT_FALSE(features.features.vertexPipelineStoresAndAtomics);
  EXPECT_FALSE(features.features.wideLines);
  EXPECT_FALSE(features.features.imageCubeArray);
  EXPECT_FALSE(features.features.shaderResourceMinLod);

  // Expect device specific things to be either true or false
  EXPECT_VK_BOOL(features.features.inheritedQueries);
  EXPECT_VK_BOOL(features.features.pipelineStatisticsQuery);
  EXPECT_VK_BOOL(features.features.robustBufferAccess);
  EXPECT_VK_BOOL(features.features.shaderImageGatherExtended);
  EXPECT_VK_BOOL(features.features.shaderSampledImageArrayDynamicIndexing);
  EXPECT_VK_BOOL(features.features.shaderStorageBufferArrayDynamicIndexing);
  EXPECT_VK_BOOL(features.features.shaderStorageImageArrayDynamicIndexing);
  EXPECT_VK_BOOL(features.features.shaderStorageImageExtendedFormats);
  EXPECT_VK_BOOL(features.features.shaderStorageImageMultisample);
  EXPECT_VK_BOOL(features.features.shaderStorageImageReadWithoutFormat);
  EXPECT_VK_BOOL(features.features.shaderStorageImageWriteWithoutFormat);
  EXPECT_VK_BOOL(features.features.shaderUniformBufferArrayDynamicIndexing);
  EXPECT_VK_BOOL(features.features.sparseBinding);
  EXPECT_VK_BOOL(features.features.sparseResidency16Samples);
  EXPECT_VK_BOOL(features.features.sparseResidency8Samples);
  EXPECT_VK_BOOL(features.features.sparseResidency4Samples);
  EXPECT_VK_BOOL(features.features.sparseResidency2Samples);
  EXPECT_VK_BOOL(features.features.sparseResidencyAliased);
  EXPECT_VK_BOOL(features.features.sparseResidencyBuffer);
  EXPECT_VK_BOOL(features.features.sparseResidencyImage2D);
  EXPECT_VK_BOOL(features.features.sparseResidencyImage3D);
  EXPECT_VK_BOOL(features.features.shaderFloat64);
  EXPECT_VK_BOOL(features.features.shaderInt16);
  EXPECT_VK_BOOL(features.features.shaderInt64);
}
