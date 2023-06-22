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

#include <vk/error.h>
#include <vk/instance.h>
#include <vk/physical_device.h>

#include <cstring>

namespace vk {
physical_device_t::physical_device_t(
    vk::instance instance, mux_device_info_t device_info,
    const compiler::Info *compiler_info,
    VkPhysicalDeviceMemoryProperties memory_properties)
    : instance(instance),
      device_info(device_info),
      compiler_info(compiler_info),
      properties(),
      memory_properties(memory_properties) {
  properties.apiVersion = VK_MAKE_VERSION(1, 0, 11);
  // TODO: decide on and implement proper versioning convention
  properties.driverVersion = 1;
  properties.vendorID = device_info->khronos_vendor_id;
  properties.deviceID = 0xC0DE91A7;

  switch (device_info->device_type) {
    case mux_device_type_e::mux_device_type_cpu:
      properties.deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;
      break;
    case mux_device_type_e::mux_device_type_gpu_integrated:
      properties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
      break;
    case mux_device_type_e::mux_device_type_gpu_discrete:
      properties.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
      break;
    case mux_device_type_e::mux_device_type_gpu_virtual:
      properties.deviceType = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
      break;
    case mux_device_type_e::mux_device_type_accelerator:
    case mux_device_type_e::mux_device_type_custom:
      properties.deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
      break;
    default:
      VK_ABORT("Unsupported device type!")
  }

  VK_ASSERT(
      std::strlen(device_info->device_name) < VK_MAX_PHYSICAL_DEVICE_NAME_SIZE,
      "Mux device name too long for Vulkan physical device name field");
  std::strncpy(properties.deviceName, device_info->device_name,
               VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);

  properties.limits = {};
  properties.limits.maxImageDimension1D = device_info->max_image_dimension_1d;
  properties.limits.maxImageDimension2D = device_info->max_image_dimension_2d;
  properties.limits.maxImageDimension3D = device_info->max_image_dimension_3d;
  properties.limits.maxMemoryAllocationCount = 4096;
  properties.limits.maxBoundDescriptorSets = 4;
  properties.limits.minMemoryMapAlignment = 4;
  properties.limits.maxPushConstantsSize = CA_VK_MAX_PUSH_CONSTANTS_SIZE;
  properties.limits.maxUniformBufferRange = 16384;
  properties.limits.maxStorageBufferRange = 134217728;
  properties.limits.maxPerStageDescriptorStorageBuffers = 4;
  properties.limits.maxPerStageDescriptorUniformBuffers = 12;
  properties.limits.maxPerStageResources = 128;
  properties.limits.maxDescriptorSetUniformBuffers = 12;
  properties.limits.maxDescriptorSetUniformBuffersDynamic = 8;
  properties.limits.maxDescriptorSetStorageBuffers = 4;
  properties.limits.maxDescriptorSetStorageBuffersDynamic = 4;
  properties.limits.maxComputeSharedMemorySize =
      device_info->shared_local_memory_size;
  properties.limits.maxComputeWorkGroupCount[0] =
      SIZE_MAX / device_info->max_work_group_size_x;
  properties.limits.maxComputeWorkGroupCount[1] =
      SIZE_MAX / device_info->max_work_group_size_y;
  properties.limits.maxComputeWorkGroupCount[2] =
      SIZE_MAX / device_info->max_work_group_size_z;
  properties.limits.maxComputeWorkGroupInvocations = 128;
  properties.limits.maxComputeWorkGroupSize[0] =
      device_info->max_work_group_size_x;
  properties.limits.maxComputeWorkGroupSize[1] =
      device_info->max_work_group_size_y;
  properties.limits.maxComputeWorkGroupSize[2] =
      device_info->max_work_group_size_z;
  properties.limits.minMemoryMapAlignment = 64;
  properties.limits.minTexelBufferOffsetAlignment =
      device_info->buffer_alignment;
  properties.limits.minUniformBufferOffsetAlignment =
      device_info->buffer_alignment;
  properties.limits.minStorageBufferOffsetAlignment =
      device_info->buffer_alignment;
  properties.limits.timestampComputeAndGraphics = VK_FALSE;
  properties.limits.discreteQueuePriorities = 2;

  // TODO: the rest of device limits

  properties.sparseProperties = {};

  // use UNIMPLEMENTED to indicate features which we should support one day
  // but which are not currently supported
  // (should be constexpr but MSVC doesn't support it)
  const VkBool32 UNIMPLEMENTED = VK_FALSE;

  // robustBufferAccess: bounds checking on buffer access; not implemented in
  // mux
  features.robustBufferAccess = UNIMPLEMENTED;

  // Unimplemented graphics features:
  features.independentBlend = VK_FALSE;
  features.geometryShader = VK_FALSE;
  features.tessellationShader = VK_FALSE;
  features.sampleRateShading = VK_FALSE;
  features.dualSrcBlend = VK_FALSE;
  features.logicOp = VK_FALSE;
  features.multiDrawIndirect = VK_FALSE;
  features.drawIndirectFirstInstance = VK_FALSE;
  features.depthClamp = VK_FALSE;
  features.depthBiasClamp = VK_FALSE;
  features.fillModeNonSolid = VK_FALSE;
  features.depthBounds = VK_FALSE;
  features.wideLines = VK_FALSE;
  features.largePoints = VK_FALSE;
  features.alphaToOne = VK_FALSE;
  features.multiViewport = VK_FALSE;
  features.samplerAnisotropy = VK_FALSE;
  features.textureCompressionETC2 = VK_FALSE;
  features.textureCompressionASTC_LDR = VK_FALSE;
  features.textureCompressionBC = VK_FALSE;
  features.occlusionQueryPrecise = VK_FALSE;
  features.fullDrawIndexUint32 = VK_FALSE;
  features.vertexPipelineStoresAndAtomics = VK_FALSE;
  features.fragmentStoresAndAtomics = VK_FALSE;
  features.shaderClipDistance = VK_FALSE;
  features.shaderCullDistance = VK_FALSE;
  features.shaderTessellationAndGeometryPointSize = VK_FALSE;
  features.variableMultisampleRate = VK_FALSE;
  features.imageCubeArray = VK_FALSE;
  features.shaderResourceMinLod = VK_FALSE;

  // The following require support for images (which we don't have yet)
  if (device_info->image_support) {
    features.shaderImageGatherExtended = UNIMPLEMENTED;
    features.shaderStorageImageExtendedFormats = UNIMPLEMENTED;
    features.shaderStorageImageMultisample = UNIMPLEMENTED;
    features.shaderStorageImageReadWithoutFormat = UNIMPLEMENTED;
    features.shaderStorageImageWriteWithoutFormat = UNIMPLEMENTED;
  } else {
    features.shaderImageGatherExtended = VK_FALSE;
    features.shaderStorageImageExtendedFormats = VK_FALSE;
    features.shaderStorageImageMultisample = VK_FALSE;
    features.shaderStorageImageReadWithoutFormat = VK_FALSE;
    features.shaderStorageImageWriteWithoutFormat = VK_FALSE;
  }

  // dynamic indexing is currently supported but likely to be device specific:
  features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
  features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
  // no image support yet though!:
  if (device_info->image_support) {
    features.shaderSampledImageArrayDynamicIndexing = UNIMPLEMENTED;
    features.shaderStorageImageArrayDynamicIndexing = UNIMPLEMENTED;
  } else {
    features.shaderSampledImageArrayDynamicIndexing = VK_FALSE;
    features.shaderStorageImageArrayDynamicIndexing = VK_FALSE;
  }

  // no support for queries currently:
  features.pipelineStatisticsQuery = UNIMPLEMENTED;
  features.inheritedQueries = UNIMPLEMENTED;

  // supported based on device capabilities
  features.shaderFloat64 =
      device_info->double_capabilities ? VK_TRUE : VK_FALSE;
  features.shaderInt64 =
      device_info->integer_capabilities & mux_integer_capabilities_64bit
          ? VK_TRUE
          : VK_FALSE;
  features.shaderInt16 =
      device_info->integer_capabilities & mux_integer_capabilities_16bit
          ? VK_TRUE
          : VK_FALSE;

  // shaderResourceResidency: requires support for images and sparse resources
  features.shaderResourceResidency = UNIMPLEMENTED;

  // support for sparse resources -> not yet implemented
  features.sparseBinding = UNIMPLEMENTED;
  features.sparseResidencyBuffer = UNIMPLEMENTED;
  features.sparseResidencyImage2D = UNIMPLEMENTED;
  features.sparseResidencyImage3D = UNIMPLEMENTED;
  features.sparseResidency2Samples = UNIMPLEMENTED;
  features.sparseResidency4Samples = UNIMPLEMENTED;
  features.sparseResidency8Samples = UNIMPLEMENTED;
  features.sparseResidency16Samples = UNIMPLEMENTED;
  features.sparseResidencyAliased = UNIMPLEMENTED;

#if CA_VK_KHR_variable_pointers
  features_variable_pointers.variablePointersStorageBuffer = VK_TRUE;
  features_variable_pointers.variablePointers = VK_TRUE;
#endif
}

physical_device_t::~physical_device_t() {}

VkResult EnumeratePhysicalDevices(vk::instance instance,
                                  uint32_t *pPhysicalDeviceCount,
                                  vk::physical_device *pPhysicalDevices) {
  if (pPhysicalDevices == nullptr) {
    *pPhysicalDeviceCount = instance->devices.size();
    return VK_SUCCESS;
  }

  for (uint32_t deviceIndex = 0, deviceIndexEnd = *pPhysicalDeviceCount;
       deviceIndex < deviceIndexEnd; deviceIndex++) {
    pPhysicalDevices[deviceIndex] = instance->devices[deviceIndex];
  }

  if (*pPhysicalDeviceCount < instance->devices.size()) {
    return VK_INCOMPLETE;
  }

  return VK_SUCCESS;
}

void GetPhysicalDeviceProperties(vk::physical_device physicalDevice,
                                 VkPhysicalDeviceProperties *pProperties) {
  *pProperties = physicalDevice->properties;
}

#if CA_VK_KHR_get_physical_device_properties2
void GetPhysicalDeviceProperties2(vk::physical_device physicalDevice,
                                  VkPhysicalDeviceProperties2 *pProperties) {
  pProperties->properties = physicalDevice->properties;
}
#endif

void GetPhysicalDeviceQueueFamilyProperties(
    vk::physical_device physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties *pQueueFamilyProperties) {
  (void)physicalDevice;
  if (pQueueFamilyProperties) {
    VkQueueFamilyProperties properties = {};
    // TODO: support multiple queues per device
    properties.queueCount = 1;
    // TODO: determine what we will actually be supporting here
    properties.queueFlags = VK_QUEUE_COMPUTE_BIT;
    pQueueFamilyProperties[0] = properties;
  } else {
    // TODO: support multiple queue families
    *pQueueFamilyPropertyCount = 1;
  }
}

#if CA_VK_KHR_get_physical_device_properties2
void GetPhysicalDeviceQueueFamilyProperties2(
    vk::physical_device physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2 *pQueueFamilyProperties) {
  (void)physicalDevice;
  if (pQueueFamilyProperties) {
    VkQueueFamilyProperties properties = {};
    properties.queueCount = 1;
    properties.queueFlags = VK_QUEUE_COMPUTE_BIT;
    pQueueFamilyProperties->queueFamilyProperties = properties;
  } else {
    *pQueueFamilyPropertyCount = 1;
  }
}
#endif

void GetPhysicalDeviceMemoryProperties(
    vk::physical_device physicalDevice,
    VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
  *pMemoryProperties = physicalDevice->memory_properties;
}

#if CA_VK_KHR_get_physical_device_properties2
void GetPhysicalDeviceMemoryProperties2(
    vk::physical_device physicalDevice,
    VkPhysicalDeviceMemoryProperties2 *pMemoryProperties) {
  pMemoryProperties->memoryProperties = physicalDevice->memory_properties;
}
#endif

void GetPhysicalDeviceFeatures(vk::physical_device physicalDevice,
                               VkPhysicalDeviceFeatures *pFeatures) {
  *pFeatures = physicalDevice->features;
}

#if CA_VK_KHR_get_physical_device_properties2
void GetPhysicalDeviceFeatures2(vk::physical_device physicalDevice,
                                VkPhysicalDeviceFeatures2 *pFeatures) {
  pFeatures->features = physicalDevice->features;

  // iterate down the pNext chain to see if there's anything in there relevant
  // to us
  void *extension_struct = pFeatures->pNext;
  while (extension_struct) {
    // all extension structs start with a common header that contains a
    // VkStructType struct type enum and the next pointer in the pNext chain
    auto header = reinterpret_cast<vk::pnext_struct_header *>(extension_struct);
    switch (header->sType) {
#if CA_VK_KHR_variable_pointers
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES:
        *reinterpret_cast<VkPhysicalDeviceVariablePointerFeatures *>(
            extension_struct) = physicalDevice->features_variable_pointers;
        break;
#endif
      default:
        break;
    }
    extension_struct = header->pNext;
  }
}
#endif

void GetPhysicalDeviceFormatProperties(vk::physical_device physicalDevice,
                                       VkFormat format,
                                       VkFormatProperties *pFormatProperties) {
  (void)physicalDevice;
  (void)format;
  (void)pFormatProperties;
}

#if CA_VK_KHR_get_physical_device_properties2
void GetPhysicalDeviceFormatProperties2(
    vk::physical_device physicalDevice, VkFormat format,
    VkFormatProperties2 *pFormatProperties) {
  (void)physicalDevice;
  (void)format;
  (void)pFormatProperties;
}
#endif

VkResult GetPhysicalDeviceImageFormatProperties(
    vk::physical_device physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkImageFormatProperties *pImageFormatProperties) {
  (void)physicalDevice;
  (void)format;
  (void)type;
  (void)tiling;
  (void)usage;
  (void)flags;
  (void)pImageFormatProperties;
  return VK_ERROR_FORMAT_NOT_SUPPORTED;
}

#if CA_VK_KHR_get_physical_device_properties2
VkResult GetPhysicalDeviceImageFormatProperties2(
    vk::physical_device physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo,
    VkImageFormatProperties2 *pImageFormatProperties) {
  (void)physicalDevice;
  (void)pImageFormatInfo;
  (void)pImageFormatProperties;
  return VK_ERROR_FEATURE_NOT_PRESENT;
}
#endif

void GetPhysicalDeviceSparseImageFormatProperties(
    vk::physical_device physicalDevice, VkFormat format, VkImageType type,
    VkSampleCountFlagBits samples, VkImageUsageFlags usage,
    VkImageTiling tiling, uint32_t *pPropertyCount,
    VkSparseImageFormatProperties *pProperties) {
  (void)physicalDevice;
  (void)format;
  (void)type;
  (void)samples;
  (void)usage;
  (void)tiling;
  (void)pPropertyCount;
  (void)pProperties;
}

#if CA_VK_KHR_get_physical_device_properties2
void GetPhysicalDeviceSparseImageFormatProperties2(
    vk::physical_device physicalDevice,
    const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo,
    uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties) {
  (void)physicalDevice;
  (void)pFormatInfo;
  (void)pPropertyCount;
  (void)pProperties;
}
#endif
}  // namespace vk
