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

#include <vk/buffer.h>
#include <vk/buffer_view.h>
#include <vk/command_buffer.h>
#include <vk/command_pool.h>
#include <vk/descriptor_pool.h>
#include <vk/descriptor_set.h>
#include <vk/descriptor_set_layout.h>
#include <vk/device.h>
#include <vk/device_memory.h>
#include <vk/event.h>
#include <vk/fence.h>
#include <vk/image.h>
#include <vk/image_view.h>
#include <vk/instance.h>
#include <vk/physical_device.h>
#include <vk/pipeline.h>
#include <vk/pipeline_cache.h>
#include <vk/pipeline_layout.h>
#include <vk/query_pool.h>
#include <vk/queue.h>
#include <vk/sampler.h>
#include <vk/semaphore.h>
#include <vk/shader_module.h>
#include <vk/type_traits.h>
#include <vulkan/vk_icd.h>

#include <cstring>

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
  return vk::CreateInstance(pCreateInfo, pAllocator,
                            vk::cast<vk::instance *>(pInstance));
}

VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
    VkInstance instance, const VkAllocationCallbacks *pAllocator) {
  vk::DestroyInstance(vk::cast<vk::instance>(instance), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                           VkPhysicalDevice *pPhysicalDevices) {
  return vk::EnumeratePhysicalDevices(
      vk::cast<vk::instance>(instance), pPhysicalDeviceCount,
      vk::cast<vk::physical_device *>(pPhysicalDevices));
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures) {
  vk::GetPhysicalDeviceFeatures(vk::cast<vk::physical_device>(physicalDevice),
                                pFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice physicalDevice, VkFormat format,
    VkFormatProperties *pFormatProperties) {
  vk::GetPhysicalDeviceFormatProperties(
      vk::cast<vk::physical_device>(physicalDevice), format, pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkImageFormatProperties *pImageFormatProperties) {
  return vk::GetPhysicalDeviceImageFormatProperties(
      vk::cast<vk::physical_device>(physicalDevice), format, type, tiling,
      usage, flags, pImageFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
  vk::GetPhysicalDeviceProperties(vk::cast<vk::physical_device>(physicalDevice),
                                  pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties *pQueueFamilyProperties) {
  vk::GetPhysicalDeviceQueueFamilyProperties(
      vk::cast<vk::physical_device>(physicalDevice), pQueueFamilyPropertyCount,
      pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
  vk::GetPhysicalDeviceMemoryProperties(
      vk::cast<vk::physical_device>(physicalDevice), pMemoryProperties);
}

#define RETURN_FUNCTION(function, name)                    \
  if (strcmp(#function, name) == 0) {                      \
    return reinterpret_cast<PFN_vkVoidFunction>(function); \
  }

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName) {
  if (instance) {
    RETURN_FUNCTION(vkCreateDevice, pName);
    RETURN_FUNCTION(vkDestroyInstance, pName);
    RETURN_FUNCTION(vkEnumeratePhysicalDevices, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceFeatures, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceFormatProperties, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceImageFormatProperties, pName);
    RETURN_FUNCTION(vkCreateDevice, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceProperties, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceMemoryProperties, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties, pName);
    RETURN_FUNCTION(vkEnumerateDeviceExtensionProperties, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceSparseImageFormatProperties, pName);
    RETURN_FUNCTION(vkGetDeviceProcAddr, pName);
  } else {
    RETURN_FUNCTION(vkCreateInstance, pName);
    RETURN_FUNCTION(vkEnumerateInstanceExtensionProperties, pName);
    RETURN_FUNCTION(vkEnumerateInstanceLayerProperties, pName);
  }
  return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
  if (instance) {
    RETURN_FUNCTION(vkCreateDevice, pName);
    RETURN_FUNCTION(vkDestroyInstance, pName);
    RETURN_FUNCTION(vkEnumeratePhysicalDevices, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceFeatures, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceFormatProperties, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceImageFormatProperties, pName);
    RETURN_FUNCTION(vkCreateDevice, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceProperties, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceMemoryProperties, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties, pName);
    RETURN_FUNCTION(vkEnumerateDeviceExtensionProperties, pName);
    RETURN_FUNCTION(vkGetPhysicalDeviceSparseImageFormatProperties, pName);
    RETURN_FUNCTION(vkGetDeviceProcAddr, pName);
    RETURN_FUNCTION(vkGetDeviceProcAddr, pName);
    RETURN_FUNCTION(vkDestroyDevice, pName);
    RETURN_FUNCTION(vkGetDeviceQueue, pName);
    RETURN_FUNCTION(vkQueueSubmit, pName);
    RETURN_FUNCTION(vkQueueWaitIdle, pName);
    RETURN_FUNCTION(vkDeviceWaitIdle, pName);
    RETURN_FUNCTION(vkAllocateMemory, pName);
    RETURN_FUNCTION(vkFreeMemory, pName);
    RETURN_FUNCTION(vkMapMemory, pName);
    RETURN_FUNCTION(vkUnmapMemory, pName);
    RETURN_FUNCTION(vkFlushMappedMemoryRanges, pName);
    RETURN_FUNCTION(vkInvalidateMappedMemoryRanges, pName);
    RETURN_FUNCTION(vkGetDeviceMemoryCommitment, pName);
    RETURN_FUNCTION(vkGetImageSparseMemoryRequirements, pName);
    RETURN_FUNCTION(vkGetBufferMemoryRequirements, pName);
    RETURN_FUNCTION(vkGetImageMemoryRequirements, pName);
    RETURN_FUNCTION(vkBindBufferMemory, pName);
    RETURN_FUNCTION(vkBindImageMemory, pName);
    RETURN_FUNCTION(vkQueueBindSparse, pName);
    RETURN_FUNCTION(vkCreateFence, pName);
    RETURN_FUNCTION(vkDestroyFence, pName);
    RETURN_FUNCTION(vkResetFences, pName);
    RETURN_FUNCTION(vkGetFenceStatus, pName);
    RETURN_FUNCTION(vkWaitForFences, pName);
    RETURN_FUNCTION(vkCreateSemaphore, pName);
    RETURN_FUNCTION(vkDestroySemaphore, pName);
    RETURN_FUNCTION(vkCreateEvent, pName);
    RETURN_FUNCTION(vkDestroyEvent, pName);
    RETURN_FUNCTION(vkGetEventStatus, pName);
    RETURN_FUNCTION(vkSetEvent, pName);
    RETURN_FUNCTION(vkResetEvent, pName);
    RETURN_FUNCTION(vkCreateQueryPool, pName);
    RETURN_FUNCTION(vkDestroyQueryPool, pName);
    RETURN_FUNCTION(vkGetQueryPoolResults, pName);
    RETURN_FUNCTION(vkCreateBuffer, pName);
    RETURN_FUNCTION(vkDestroyBuffer, pName);
    RETURN_FUNCTION(vkCreateBufferView, pName);
    RETURN_FUNCTION(vkDestroyBufferView, pName);
    RETURN_FUNCTION(vkCreateImage, pName);
    RETURN_FUNCTION(vkDestroyImage, pName);
    RETURN_FUNCTION(vkGetImageSubresourceLayout, pName);
    RETURN_FUNCTION(vkCreateImageView, pName);
    RETURN_FUNCTION(vkDestroyImageView, pName);
    RETURN_FUNCTION(vkCreateShaderModule, pName);
    RETURN_FUNCTION(vkDestroyShaderModule, pName);
    RETURN_FUNCTION(vkCreatePipelineCache, pName);
    RETURN_FUNCTION(vkDestroyPipelineCache, pName);
    RETURN_FUNCTION(vkGetPipelineCacheData, pName);
    RETURN_FUNCTION(vkMergePipelineCaches, pName);
    RETURN_FUNCTION(vkCreateGraphicsPipelines, pName);
    RETURN_FUNCTION(vkCreateComputePipelines, pName);
    RETURN_FUNCTION(vkDestroyPipeline, pName);
    RETURN_FUNCTION(vkCreatePipelineLayout, pName);
    RETURN_FUNCTION(vkDestroyPipelineLayout, pName);
    RETURN_FUNCTION(vkCreateSampler, pName);
    RETURN_FUNCTION(vkDestroySampler, pName);
    RETURN_FUNCTION(vkCreateDescriptorSetLayout, pName);
    RETURN_FUNCTION(vkDestroyDescriptorSetLayout, pName);
    RETURN_FUNCTION(vkCreateDescriptorPool, pName);
    RETURN_FUNCTION(vkDestroyDescriptorPool, pName);
    RETURN_FUNCTION(vkResetDescriptorPool, pName);
    RETURN_FUNCTION(vkAllocateDescriptorSets, pName);
    RETURN_FUNCTION(vkFreeDescriptorSets, pName);
    RETURN_FUNCTION(vkUpdateDescriptorSets, pName);
    RETURN_FUNCTION(vkCreateFramebuffer, pName);
    RETURN_FUNCTION(vkDestroyFramebuffer, pName);
    RETURN_FUNCTION(vkCreateRenderPass, pName);
    RETURN_FUNCTION(vkDestroyRenderPass, pName);
    RETURN_FUNCTION(vkGetRenderAreaGranularity, pName);
    RETURN_FUNCTION(vkCreateCommandPool, pName);
    RETURN_FUNCTION(vkDestroyCommandPool, pName);
    RETURN_FUNCTION(vkResetCommandPool, pName);
    RETURN_FUNCTION(vkAllocateCommandBuffers, pName);
    RETURN_FUNCTION(vkFreeCommandBuffers, pName);
    RETURN_FUNCTION(vkBeginCommandBuffer, pName);
    RETURN_FUNCTION(vkEndCommandBuffer, pName);
    RETURN_FUNCTION(vkResetCommandBuffer, pName);
    RETURN_FUNCTION(vkCmdBindPipeline, pName);
    RETURN_FUNCTION(vkCmdSetViewport, pName);
    RETURN_FUNCTION(vkCmdSetScissor, pName);
    RETURN_FUNCTION(vkCmdSetLineWidth, pName);
    RETURN_FUNCTION(vkCmdSetDepthBias, pName);
    RETURN_FUNCTION(vkCmdSetBlendConstants, pName);
    RETURN_FUNCTION(vkCmdSetDepthBounds, pName);
    RETURN_FUNCTION(vkCmdSetStencilCompareMask, pName);
    RETURN_FUNCTION(vkCmdSetStencilWriteMask, pName);
    RETURN_FUNCTION(vkCmdSetStencilReference, pName);
    RETURN_FUNCTION(vkCmdBindDescriptorSets, pName);
    RETURN_FUNCTION(vkCmdBindVertexBuffers, pName);
    RETURN_FUNCTION(vkCmdBindIndexBuffer, pName);
    RETURN_FUNCTION(vkCmdDraw, pName);
    RETURN_FUNCTION(vkCmdDrawIndexed, pName);
    RETURN_FUNCTION(vkCmdDrawIndirect, pName);
    RETURN_FUNCTION(vkCmdDrawIndexedIndirect, pName);
    RETURN_FUNCTION(vkCmdDispatch, pName);
    RETURN_FUNCTION(vkCmdDispatchIndirect, pName);
    RETURN_FUNCTION(vkCmdCopyBuffer, pName);
    RETURN_FUNCTION(vkCmdCopyImage, pName);
    RETURN_FUNCTION(vkCmdBlitImage, pName);
    RETURN_FUNCTION(vkCmdCopyBufferToImage, pName);
    RETURN_FUNCTION(vkCmdCopyImageToBuffer, pName);
    RETURN_FUNCTION(vkCmdUpdateBuffer, pName);
    RETURN_FUNCTION(vkCmdFillBuffer, pName);
    RETURN_FUNCTION(vkCmdClearColorImage, pName);
    RETURN_FUNCTION(vkCmdClearDepthStencilImage, pName);
    RETURN_FUNCTION(vkCmdClearAttachments, pName);
    RETURN_FUNCTION(vkCmdResolveImage, pName);
    RETURN_FUNCTION(vkCmdSetEvent, pName);
    RETURN_FUNCTION(vkCmdResetEvent, pName);
    RETURN_FUNCTION(vkCmdWaitEvents, pName);
    RETURN_FUNCTION(vkCmdPipelineBarrier, pName);
    RETURN_FUNCTION(vkCmdBeginQuery, pName);
    RETURN_FUNCTION(vkCmdEndQuery, pName);
    RETURN_FUNCTION(vkCmdResetQueryPool, pName);
    RETURN_FUNCTION(vkCmdWriteTimestamp, pName);
    RETURN_FUNCTION(vkCmdCopyQueryPoolResults, pName);
    RETURN_FUNCTION(vkCmdPushConstants, pName);
    RETURN_FUNCTION(vkCmdBeginRenderPass, pName);
    RETURN_FUNCTION(vkCmdNextSubpass, pName);
    RETURN_FUNCTION(vkCmdEndRenderPass, pName);
    RETURN_FUNCTION(vkCmdExecuteCommands, pName);
  } else {
    RETURN_FUNCTION(vkCreateInstance, pName);
    RETURN_FUNCTION(vkEnumerateInstanceExtensionProperties, pName);
    RETURN_FUNCTION(vkEnumerateInstanceLayerProperties, pName);
  }
  return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetDeviceProcAddr(VkDevice device, const char *pName) {
  (void)device;
  RETURN_FUNCTION(vkGetDeviceProcAddr, pName);
  RETURN_FUNCTION(vkDestroyDevice, pName);
  RETURN_FUNCTION(vkGetDeviceQueue, pName);
  RETURN_FUNCTION(vkQueueSubmit, pName);
  RETURN_FUNCTION(vkQueueWaitIdle, pName);
  RETURN_FUNCTION(vkDeviceWaitIdle, pName);
  RETURN_FUNCTION(vkAllocateMemory, pName);
  RETURN_FUNCTION(vkFreeMemory, pName);
  RETURN_FUNCTION(vkMapMemory, pName);
  RETURN_FUNCTION(vkUnmapMemory, pName);
  RETURN_FUNCTION(vkFlushMappedMemoryRanges, pName);
  RETURN_FUNCTION(vkInvalidateMappedMemoryRanges, pName);
  RETURN_FUNCTION(vkGetDeviceMemoryCommitment, pName);
  RETURN_FUNCTION(vkGetImageSparseMemoryRequirements, pName);
  RETURN_FUNCTION(vkGetBufferMemoryRequirements, pName);
  RETURN_FUNCTION(vkGetImageMemoryRequirements, pName);
  RETURN_FUNCTION(vkBindBufferMemory, pName);
  RETURN_FUNCTION(vkBindImageMemory, pName);
  RETURN_FUNCTION(vkQueueBindSparse, pName);
  RETURN_FUNCTION(vkCreateFence, pName);
  RETURN_FUNCTION(vkDestroyFence, pName);
  RETURN_FUNCTION(vkResetFences, pName);
  RETURN_FUNCTION(vkGetFenceStatus, pName);
  RETURN_FUNCTION(vkWaitForFences, pName);
  RETURN_FUNCTION(vkCreateSemaphore, pName);
  RETURN_FUNCTION(vkDestroySemaphore, pName);
  RETURN_FUNCTION(vkCreateEvent, pName);
  RETURN_FUNCTION(vkDestroyEvent, pName);
  RETURN_FUNCTION(vkGetEventStatus, pName);
  RETURN_FUNCTION(vkSetEvent, pName);
  RETURN_FUNCTION(vkResetEvent, pName);
  RETURN_FUNCTION(vkCreateQueryPool, pName);
  RETURN_FUNCTION(vkDestroyQueryPool, pName);
  RETURN_FUNCTION(vkGetQueryPoolResults, pName);
  RETURN_FUNCTION(vkCreateBuffer, pName);
  RETURN_FUNCTION(vkDestroyBuffer, pName);
  RETURN_FUNCTION(vkCreateBufferView, pName);
  RETURN_FUNCTION(vkDestroyBufferView, pName);
  RETURN_FUNCTION(vkCreateImage, pName);
  RETURN_FUNCTION(vkDestroyImage, pName);
  RETURN_FUNCTION(vkGetImageSubresourceLayout, pName);
  RETURN_FUNCTION(vkCreateImageView, pName);
  RETURN_FUNCTION(vkDestroyImageView, pName);
  RETURN_FUNCTION(vkCreateShaderModule, pName);
  RETURN_FUNCTION(vkDestroyShaderModule, pName);
  RETURN_FUNCTION(vkCreatePipelineCache, pName);
  RETURN_FUNCTION(vkDestroyPipelineCache, pName);
  RETURN_FUNCTION(vkGetPipelineCacheData, pName);
  RETURN_FUNCTION(vkMergePipelineCaches, pName);
  RETURN_FUNCTION(vkCreateGraphicsPipelines, pName);
  RETURN_FUNCTION(vkCreateComputePipelines, pName);
  RETURN_FUNCTION(vkDestroyPipeline, pName);
  RETURN_FUNCTION(vkCreatePipelineLayout, pName);
  RETURN_FUNCTION(vkDestroyPipelineLayout, pName);
  RETURN_FUNCTION(vkCreateSampler, pName);
  RETURN_FUNCTION(vkDestroySampler, pName);
  RETURN_FUNCTION(vkCreateDescriptorSetLayout, pName);
  RETURN_FUNCTION(vkDestroyDescriptorSetLayout, pName);
  RETURN_FUNCTION(vkCreateDescriptorPool, pName);
  RETURN_FUNCTION(vkDestroyDescriptorPool, pName);
  RETURN_FUNCTION(vkResetDescriptorPool, pName);
  RETURN_FUNCTION(vkAllocateDescriptorSets, pName);
  RETURN_FUNCTION(vkFreeDescriptorSets, pName);
  RETURN_FUNCTION(vkUpdateDescriptorSets, pName);
  RETURN_FUNCTION(vkCreateFramebuffer, pName);
  RETURN_FUNCTION(vkDestroyFramebuffer, pName);
  RETURN_FUNCTION(vkCreateRenderPass, pName);
  RETURN_FUNCTION(vkDestroyRenderPass, pName);
  RETURN_FUNCTION(vkGetRenderAreaGranularity, pName);
  RETURN_FUNCTION(vkCreateCommandPool, pName);
  RETURN_FUNCTION(vkDestroyCommandPool, pName);
  RETURN_FUNCTION(vkResetCommandPool, pName);
  RETURN_FUNCTION(vkAllocateCommandBuffers, pName);
  RETURN_FUNCTION(vkFreeCommandBuffers, pName);
  RETURN_FUNCTION(vkBeginCommandBuffer, pName);
  RETURN_FUNCTION(vkEndCommandBuffer, pName);
  RETURN_FUNCTION(vkResetCommandBuffer, pName);
  RETURN_FUNCTION(vkCmdBindPipeline, pName);
  RETURN_FUNCTION(vkCmdSetViewport, pName);
  RETURN_FUNCTION(vkCmdSetScissor, pName);
  RETURN_FUNCTION(vkCmdSetLineWidth, pName);
  RETURN_FUNCTION(vkCmdSetDepthBias, pName);
  RETURN_FUNCTION(vkCmdSetBlendConstants, pName);
  RETURN_FUNCTION(vkCmdSetDepthBounds, pName);
  RETURN_FUNCTION(vkCmdSetStencilCompareMask, pName);
  RETURN_FUNCTION(vkCmdSetStencilWriteMask, pName);
  RETURN_FUNCTION(vkCmdSetStencilReference, pName);
  RETURN_FUNCTION(vkCmdBindDescriptorSets, pName);
  RETURN_FUNCTION(vkCmdBindVertexBuffers, pName);
  RETURN_FUNCTION(vkCmdBindIndexBuffer, pName);
  RETURN_FUNCTION(vkCmdDraw, pName);
  RETURN_FUNCTION(vkCmdDrawIndexed, pName);
  RETURN_FUNCTION(vkCmdDrawIndirect, pName);
  RETURN_FUNCTION(vkCmdDrawIndexedIndirect, pName);
  RETURN_FUNCTION(vkCmdDispatch, pName);
  RETURN_FUNCTION(vkCmdDispatchIndirect, pName);
  RETURN_FUNCTION(vkCmdCopyBuffer, pName);
  RETURN_FUNCTION(vkCmdCopyImage, pName);
  RETURN_FUNCTION(vkCmdBlitImage, pName);
  RETURN_FUNCTION(vkCmdCopyBufferToImage, pName);
  RETURN_FUNCTION(vkCmdCopyImageToBuffer, pName);
  RETURN_FUNCTION(vkCmdUpdateBuffer, pName);
  RETURN_FUNCTION(vkCmdFillBuffer, pName);
  RETURN_FUNCTION(vkCmdClearColorImage, pName);
  RETURN_FUNCTION(vkCmdClearDepthStencilImage, pName);
  RETURN_FUNCTION(vkCmdClearAttachments, pName);
  RETURN_FUNCTION(vkCmdResolveImage, pName);
  RETURN_FUNCTION(vkCmdSetEvent, pName);
  RETURN_FUNCTION(vkCmdResetEvent, pName);
  RETURN_FUNCTION(vkCmdWaitEvents, pName);
  RETURN_FUNCTION(vkCmdPipelineBarrier, pName);
  RETURN_FUNCTION(vkCmdBeginQuery, pName);
  RETURN_FUNCTION(vkCmdEndQuery, pName);
  RETURN_FUNCTION(vkCmdResetQueryPool, pName);
  RETURN_FUNCTION(vkCmdWriteTimestamp, pName);
  RETURN_FUNCTION(vkCmdCopyQueryPoolResults, pName);
  RETURN_FUNCTION(vkCmdPushConstants, pName);
  RETURN_FUNCTION(vkCmdBeginRenderPass, pName);
  RETURN_FUNCTION(vkCmdNextSubpass, pName);
  RETURN_FUNCTION(vkCmdEndRenderPass, pName);
  RETURN_FUNCTION(vkCmdExecuteCommands, pName);
  return nullptr;
}

#undef RETURN_FUNCTION

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
  return vk::CreateDevice(vk::cast<vk::physical_device>(physicalDevice),
                          pCreateInfo, pAllocator,
                          vk::cast<vk::device *>(pDevice));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
  vk::DestroyDevice(vk::cast<vk::device>(device), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
    const char *pLayerName, uint32_t *pPropertyCount,
    VkExtensionProperties *pProperties) {
  return vk::EnumerateInstanceExtensionProperties(pLayerName, pPropertyCount,
                                                  pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice, const char *pLayerName,
    uint32_t *pPropertyCount, VkExtensionProperties *pProperties) {
  return vk::EnumerateDeviceExtensionProperties(
      vk::cast<vk::physical_device>(physicalDevice), pLayerName, pPropertyCount,
      pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
    uint32_t *pPropertyCount, VkLayerProperties *pProperties) {
  (void)pProperties;

  // this implementation does not provide any layers
  *pPropertyCount = 0;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
    VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
    VkLayerProperties *pProperties) {
  (void)physicalDevice;
  (void)pProperties;

  // this implementation does not provide any layers
  *pPropertyCount = 0;
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device,
                                            uint32_t queueFamilyIndex,
                                            uint32_t queueIndex,
                                            VkQueue *pQueue) {
  vk::GetDeviceQueue(vk::cast<vk::device>(device), queueFamilyIndex, queueIndex,
                     vk::cast<vk::queue *>(pQueue));
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue,
                                             uint32_t submitCount,
                                             const VkSubmitInfo *pSubmits,
                                             VkFence fence) {
  return vk::QueueSubmit(vk::cast<vk::queue>(queue), submitCount, pSubmits,
                         vk::cast<vk::fence>(fence));
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(VkQueue queue) {
  return vk::QueueWaitIdle(vk::cast<vk::queue>(queue));
}

VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice device) {
  return vk::DeviceWaitIdle(vk::cast<vk::device>(device));
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(
    VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo,
    const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
  return vk::AllocateMemory(vk::cast<vk::device>(device), pAllocateInfo,
                            pAllocator, vk::cast<vk::device_memory *>(pMemory));
}

VKAPI_ATTR void VKAPI_CALL
vkFreeMemory(VkDevice device, VkDeviceMemory memory,
             const VkAllocationCallbacks *pAllocator) {
  vk::FreeMemory(vk::cast<vk::device>(device),
                 vk::cast<vk::device_memory>(memory), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset,
            VkDeviceSize size, VkMemoryMapFlags flags, void **ppData) {
  return vk::MapMemory(vk::cast<vk::device>(device),
                       vk::cast<vk::device_memory>(memory), offset, size, flags,
                       ppData);
}

VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice device,
                                         VkDeviceMemory memory) {
  vk::UnmapMemory(vk::cast<vk::device>(device),
                  vk::cast<vk::device_memory>(memory));
}

VKAPI_ATTR VkResult VKAPI_CALL
vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                          const VkMappedMemoryRange *pMemoryRanges) {
  return vk::FlushMemoryMappedRanges(vk::cast<vk::device>(device),
                                     memoryRangeCount, pMemoryRanges);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                               const VkMappedMemoryRange *pMemoryRanges) {
  return vk::InvalidateMemoryMappedRanges(vk::cast<vk::device>(device),
                                          memoryRangeCount, pMemoryRanges);
}

VKAPI_ATTR void VKAPI_CALL
vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory,
                            VkDeviceSize *pCommittedMemoryInBytes) {
  vk::GetDeviceMemoryCommitment(vk::cast<vk::device>(device),
                                vk::cast<vk::device_memory>(memory),
                                pCommittedMemoryInBytes);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice device,
                                                  VkBuffer buffer,
                                                  VkDeviceMemory memory,
                                                  VkDeviceSize memoryOffset) {
  return vk::BindBufferMemory(
      vk::cast<vk::device>(device), vk::cast<vk::buffer>(buffer),
      vk::cast<vk::device_memory>(memory), memoryOffset);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(VkDevice device, VkImage image,
                                                 VkDeviceMemory memory,
                                                 VkDeviceSize memoryOffset) {
  return vk::BindImageMemory(vk::cast<vk::device>(device), image,
                             vk::cast<vk::device_memory>(memory), memoryOffset);
}

VKAPI_ATTR void VKAPI_CALL
vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer,
                              VkMemoryRequirements *pMemoryRequirements) {
  vk::GetBufferMemoryRequirements(vk::cast<vk::device>(device),
                                  vk::cast<vk::buffer>(buffer),
                                  pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(
    VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements) {
  vk::GetImageMemoryRequirements(vk::cast<vk::device>(device),
                                 vk::cast<vk::image>(image),
                                 pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements(
    VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount,
    VkSparseImageMemoryRequirements *pSparseMemoryRequirements) {
  vk::GetImageSparseMemoryRequirements(
      vk::cast<vk::device>(device), vk::cast<vk::image>(image),
      pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkSampleCountFlagBits samples, VkImageUsageFlags usage,
    VkImageTiling tiling, uint32_t *pPropertyCount,
    VkSparseImageFormatProperties *pProperties) {
  vk::GetPhysicalDeviceSparseImageFormatProperties(
      vk::cast<vk::physical_device>(physicalDevice), format, type, samples,
      usage, tiling, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount,
                  const VkBindSparseInfo *pBindInfo, VkFence fence) {
  return vk::QueueBindSparse(vk::cast<vk::queue>(queue), bindInfoCount,
                             pBindInfo, vk::cast<vk::fence>(fence));
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo,
              const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
  return vk::CreateFence(vk::cast<vk::device>(device), pCreateInfo, pAllocator,
                         vk::cast<vk::fence *>(pFence));
}

VKAPI_ATTR void VKAPI_CALL vkDestroyFence(
    VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) {
  vk::DestroyFence(vk::cast<vk::device>(device), vk::cast<vk::fence>(fence),
                   pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(VkDevice device,
                                             uint32_t fenceCount,
                                             const VkFence *pFences) {
  return vk::ResetFences(vk::cast<vk::device>(device), fenceCount,
                         vk::cast<const vk::fence *>(pFences));
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(VkDevice device,
                                                VkFence fence) {
  return vk::GetFenceStatus(vk::cast<vk::device>(device),
                            vk::cast<vk::fence>(fence));
}

VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(VkDevice device,
                                               uint32_t fenceCount,
                                               const VkFence *pFences,
                                               VkBool32 waitAll,
                                               uint64_t timeout) {
  return vk::WaitForFences(vk::cast<vk::device>(device), fenceCount, pFences,
                           waitAll, timeout);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(
    VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) {
  return vk::CreateSemaphore(vk::cast<vk::device>(device), pCreateInfo,
                             pAllocator, vk::cast<vk::semaphore *>(pSemaphore));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroySemaphore(VkDevice device, VkSemaphore semaphore,
                   const VkAllocationCallbacks *pAllocator) {
  vk::DestroySemaphore(vk::cast<vk::device>(device),
                       vk::cast<vk::semaphore>(semaphore), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo,
              const VkAllocationCallbacks *pAllocator, VkEvent *pEvent) {
  return vk::CreateEvent(vk::cast<vk::device>(device), pCreateInfo, pAllocator,
                         vk::cast<vk::event *>(pEvent));
}

VKAPI_ATTR void VKAPI_CALL vkDestroyEvent(
    VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator) {
  vk::DestroyEvent(vk::cast<vk::device>(device), vk::cast<vk::event>(event),
                   pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetEventStatus(VkDevice device,
                                                VkEvent event) {
  return vk::GetEventStatus(vk::cast<vk::device>(device),
                            vk::cast<vk::event>(event));
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent(VkDevice device, VkEvent event) {
  return vk::SetEvent(vk::cast<vk::device>(device), vk::cast<vk::event>(event));
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetEvent(VkDevice device, VkEvent event) {
  return vk::ResetEvent(vk::cast<vk::device>(device),
                        vk::cast<vk::event>(event));
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(
    VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool) {
  return vk::CreateQueryPool(vk::cast<vk::device>(device), pCreateInfo,
                             pAllocator,
                             vk::cast<vk::query_pool *>(pQueryPool));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool,
                   const VkAllocationCallbacks *pAllocator) {
  vk::DestroyQueryPool(vk::cast<vk::device>(device),
                       vk::cast<vk::query_pool>(queryPool), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(
    VkDevice device, VkQueryPool queryPool, uint32_t firstQuery,
    uint32_t queryCount, size_t dataSize, void *pData, VkDeviceSize stride,
    VkQueryResultFlags flags) {
  return vk::GetQueryPoolResults(
      vk::cast<vk::device>(device), vk::cast<vk::query_pool>(queryPool),
      firstQuery, queryCount, dataSize, pData, stride, flags);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo,
               const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer) {
  return vk::CreateBuffer(vk::cast<vk::device>(device), pCreateInfo, pAllocator,
                          vk::cast<vk::buffer *>(pBuffer));
}

VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(
    VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator) {
  vk::DestroyBuffer(vk::cast<vk::device>(device), vk::cast<vk::buffer>(buffer),
                    pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(
    VkDevice device, const VkBufferViewCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkBufferView *pView) {
  return vk::CreateBufferView(vk::cast<vk::device>(device), pCreateInfo,
                              pAllocator, vk::cast<vk::buffer_view *>(pView));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyBufferView(VkDevice device, VkBufferView bufferView,
                    const VkAllocationCallbacks *pAllocator) {
  vk::DestroyBufferView(vk::cast<vk::device>(device),
                        vk::cast<vk::buffer_view>(bufferView), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
              const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
  return vk::CreateImage(vk::cast<vk::device>(device), pCreateInfo, pAllocator,
                         vk::cast<vk::image *>(pImage));
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImage(
    VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator) {
  vk::DestroyImage(vk::cast<vk::device>(device), vk::cast<vk::image>(image),
                   pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout(
    VkDevice device, VkImage image, const VkImageSubresource *pSubresource,
    VkSubresourceLayout *pLayout) {
  vk::GetImageSubresourceLayout(vk::cast<vk::device>(device),
                                vk::cast<vk::image>(image), pSubresource,
                                pLayout);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                  const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
  return vk::CreateImageView(vk::cast<vk::device>(device), pCreateInfo,
                             pAllocator, vk::cast<vk::image_view *>(pView));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyImageView(VkDevice device, VkImageView imageView,
                   const VkAllocationCallbacks *pAllocator) {
  vk::DestroyImageView(vk::cast<vk::device>(device),
                       vk::cast<vk::image_view>(imageView), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(
    VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule) {
  return vk::CreateShaderModule(vk::cast<vk::device>(device), pCreateInfo,
                                pAllocator,
                                vk::cast<vk::shader_module *>(pShaderModule));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule,
                      const VkAllocationCallbacks *pAllocator) {
  vk::DestroyShaderModule(vk::cast<vk::device>(device),
                          vk::cast<vk::shader_module>(shaderModule),
                          pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(
    VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache) {
  return vk::CreatePipelineCache(
      vk::cast<vk::device>(device), pCreateInfo, pAllocator,
      vk::cast<vk::pipeline_cache *>(pPipelineCache));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache,
                       const VkAllocationCallbacks *pAllocator) {
  vk::DestroyPipelineCache(vk::cast<vk::device>(device),
                           vk::cast<vk::pipeline_cache>(pipelineCache),
                           pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache,
                       size_t *pDataSize, void *pData) {
  return vk::GetPipelineCacheData(vk::cast<vk::device>(device),
                                  vk::cast<vk::pipeline_cache>(pipelineCache),
                                  pDataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(
    VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount,
    const VkPipelineCache *pSrcCaches) {
  return vk::MergePipelineCaches(vk::cast<vk::device>(device),
                                 vk::cast<vk::pipeline_cache>(dstCache),
                                 srcCacheCount, pSrcCaches);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(
    VkDevice, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo *,
    const VkAllocationCallbacks *, VkPipeline *) {
  // no stub since graphics operations will never be supported
  return VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(
    VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkComputePipelineCreateInfo *pCreateInfos,
    const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
  return vk::CreateComputePipelines(
      vk::cast<vk::device>(device), vk::cast<vk::pipeline_cache>(pipelineCache),
      createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyPipeline(VkDevice device, VkPipeline pipeline,
                  const VkAllocationCallbacks *pAllocator) {
  vk::DestroyPipeline(vk::cast<vk::device>(device),
                      vk::cast<vk::pipeline>(pipeline), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(
    VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkPipelineLayout *pPipelineLayout) {
  return vk::CreatePipelineLayout(
      vk::cast<vk::device>(device), pCreateInfo, pAllocator,
      vk::cast<vk::pipeline_layout *>(pPipelineLayout));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout,
                        const VkAllocationCallbacks *pAllocator) {
  vk::DestroyPipelineLayout(vk::cast<vk::device>(device),
                            vk::cast<vk::pipeline_layout>(pipelineLayout),
                            pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo,
                const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) {
  return vk::CreateSampler(vk::cast<vk::device>(device), pCreateInfo,
                           pAllocator, vk::cast<vk::sampler *>(pSampler));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroySampler(VkDevice device, VkSampler sampler,
                 const VkAllocationCallbacks *pAllocator) {
  vk::DestroySampler(vk::cast<vk::device>(device),
                     vk::cast<vk::sampler>(sampler), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(
    VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDescriptorSetLayout *pSetLayout) {
  return vk::CreateDescriptorSetLayout(
      vk::cast<vk::device>(device), pCreateInfo, pAllocator,
      vk::cast<vk::descriptor_set_layout *>(pSetLayout));
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
    const VkAllocationCallbacks *pAllocator) {
  vk::DestroyDescriptorSetLayout(
      vk::cast<vk::device>(device),
      vk::cast<vk::descriptor_set_layout>(descriptorSetLayout), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(
    VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDescriptorPool *pDescriptorPool) {
  return vk::CreateDescriptorPool(
      vk::cast<vk::device>(device), pCreateInfo, vk::allocator(pAllocator),
      vk::cast<vk::descriptor_pool *>(pDescriptorPool));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                        const VkAllocationCallbacks *pAllocator) {
  vk::DestroyDescriptorPool(vk::cast<vk::device>(device),
                            vk::cast<vk::descriptor_pool>(descriptorPool),
                            pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                      VkDescriptorPoolResetFlags flags) {
  return vk::ResetDescriptorPool(vk::cast<vk::device>(device),
                                 vk::cast<vk::descriptor_pool>(descriptorPool),
                                 flags);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(
    VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
    VkDescriptorSet *pDescriptorSets) {
  return vk::AllocateDescriptorSets(vk::cast<vk::device>(device), pAllocateInfo,
                                    pDescriptorSets);
}

VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(
    VkDevice device, VkDescriptorPool descriptorPool,
    uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets) {
  return vk::FreeDescriptorSets(
      vk::cast<vk::device>(device),
      vk::cast<vk::descriptor_pool>(descriptorPool), descriptorSetCount,
      vk::cast<const vk::descriptor_set *>(pDescriptorSets));
}

VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(
    VkDevice device, uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
    const VkCopyDescriptorSet *pDescriptorCopies) {
  vk::UpdateDescriptorSets(vk::cast<vk::device>(device), descriptorWriteCount,
                           pDescriptorWrites, descriptorCopyCount,
                           pDescriptorCopies);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateFramebuffer(VkDevice, const VkFramebufferCreateInfo *,
                    const VkAllocationCallbacks *, VkFramebuffer *) {
  // no stub since graphics operations will never be supported
  return VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(VkDevice, VkFramebuffer,
                                                const VkAllocationCallbacks *) {
  // no stub since graphics operations will never be supported
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateRenderPass(VkDevice, const VkRenderPassCreateInfo *,
                   const VkAllocationCallbacks *, VkRenderPass *) {
  // no stub since graphics operations will never be supported
  return VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(VkDevice, VkRenderPass,
                                               const VkAllocationCallbacks *) {
  // no stub since graphics operations will never be supported
}

VKAPI_ATTR void VKAPI_CALL vkGetRenderAreaGranularity(VkDevice, VkRenderPass,
                                                      VkExtent2D *) {
  // no stub since graphics operations will never be supported
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(
    VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool) {
  return vk::CreateCommandPool(vk::cast<vk::device>(device), pCreateInfo,
                               pAllocator,
                               vk::cast<vk::command_pool *>(pCommandPool));
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool,
                     const VkAllocationCallbacks *pAllocator) {
  vk::DestroyCommandPool(vk::cast<vk::device>(device),
                         vk::cast<vk::command_pool>(commandPool), pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(
    VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) {
  return vk::ResetCommandPool(vk::cast<vk::device>(device),
                              vk::cast<vk::command_pool>(commandPool), flags);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo,
    VkCommandBuffer *pCommandBuffers) {
  return vk::AllocateCommandBuffers(
      vk::cast<vk::device>(device), pAllocateInfo,
      vk::cast<vk::command_buffer *>(pCommandBuffers));
}

VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(
    VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
    const VkCommandBuffer *pCommandBuffers) {
  vk::FreeCommandBuffers(vk::cast<vk::device>(device),
                         vk::cast<vk::command_pool>(commandPool),
                         commandBufferCount,
                         vk::cast<const vk::command_buffer *>(pCommandBuffers));
}

VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
    VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
  return vk::BeginCommandBuffer(vk::cast<vk::command_buffer>(commandBuffer),
                                pBeginInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkEndCommandBuffer(VkCommandBuffer commandBuffer) {
  return vk::EndCommandBuffer(vk::cast<vk::command_buffer>(commandBuffer));
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(
    VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
  return vk::ResetCommandBuffer(vk::cast<vk::command_buffer>(commandBuffer),
                                flags);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdBindPipeline(VkCommandBuffer commandBuffer,
                  VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
  vk::CmdBindPipeline(vk::cast<vk::command_buffer>(commandBuffer),
                      pipelineBindPoint, vk::cast<vk::pipeline>(pipeline));
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(VkCommandBuffer commandBuffer,
                                            uint32_t, uint32_t,
                                            const VkViewport *) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(VkCommandBuffer commandBuffer,
                                           uint32_t, uint32_t,
                                           const VkRect2D *) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth(VkCommandBuffer commandBuffer,
                                             float) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias(VkCommandBuffer commandBuffer,
                                             float, float, float) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants(VkCommandBuffer commandBuffer,
                                                  const float[4]) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds(VkCommandBuffer commandBuffer,
                                               float, float) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask(
    VkCommandBuffer commandBuffer, VkStencilFaceFlags, uint32_t) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask(
    VkCommandBuffer commandBuffer, VkStencilFaceFlags, uint32_t) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference(
    VkCommandBuffer commandBuffer, VkStencilFaceFlags, uint32_t) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(
    VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
    const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount,
    const uint32_t *pDynamicOffsets) {
  vk::CmdBindDescriptorSets(
      vk::cast<vk::command_buffer>(commandBuffer), pipelineBindPoint,
      vk::cast<vk::pipeline_layout>(layout), firstSet, descriptorSetCount,
      pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer,
                                                VkBuffer, VkDeviceSize,
                                                VkIndexType) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer,
                                                  uint32_t, uint32_t,
                                                  const VkBuffer *,
                                                  const VkDeviceSize *) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t,
                                     uint32_t, uint32_t, uint32_t) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(VkCommandBuffer commandBuffer,
                                            uint32_t, uint32_t, uint32_t,
                                            int32_t, uint32_t) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect(VkCommandBuffer commandBuffer,
                                             VkBuffer, VkDeviceSize, uint32_t,
                                             uint32_t) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect(
    VkCommandBuffer commandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(VkCommandBuffer commandBuffer,
                                         uint32_t x, uint32_t y, uint32_t z) {
  vk::CmdDispatch(vk::cast<vk::command_buffer>(commandBuffer), x, y, z);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(VkCommandBuffer commandBuffer,
                                                 VkBuffer buffer,
                                                 VkDeviceSize offset) {
  vk::CmdDispatchIndirect(vk::cast<vk::command_buffer>(commandBuffer),
                          vk::cast<vk::buffer>(buffer), offset);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(VkCommandBuffer commandBuffer,
                                           VkBuffer srcBuffer,
                                           VkBuffer dstBuffer,
                                           uint32_t regionCount,
                                           const VkBufferCopy *pRegions) {
  vk::CmdCopyBuffer(vk::cast<vk::command_buffer>(commandBuffer),
                    vk::cast<vk::buffer>(srcBuffer),
                    vk::cast<vk::buffer>(dstBuffer), regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(VkCommandBuffer commandBuffer,
                                          VkImage srcImage,
                                          VkImageLayout srcImageLayout,
                                          VkImage dstImage,
                                          VkImageLayout dstImageLayout,
                                          uint32_t regionCount,
                                          const VkImageCopy *pRegions) {
  vk::CmdCopyImage(vk::cast<vk::command_buffer>(commandBuffer),
                   vk::cast<vk::image>(srcImage), srcImageLayout,
                   vk::cast<vk::image>(dstImage), dstImageLayout, regionCount,
                   pRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(VkCommandBuffer commandBuffer,
                                          VkImage, VkImageLayout, VkImage,
                                          VkImageLayout, uint32_t,
                                          const VkImageBlit *, VkFilter) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(
    VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
    VkImageLayout dstImageLayout, uint32_t regionCount,
    const VkBufferImageCopy *pRegions) {
  vk::CmdCopyBufferToImage(vk::cast<vk::command_buffer>(commandBuffer),
                           vk::cast<vk::buffer>(srcBuffer),
                           vk::cast<vk::image>(dstImage), dstImageLayout,
                           regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(
    VkCommandBuffer commandBuffer, VkImage srcImage,
    VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount,
    const VkBufferImageCopy *pRegions) {
  vk::CmdCopyImageToBuffer(vk::cast<vk::command_buffer>(commandBuffer),
                           vk::cast<vk::image>(srcImage), srcImageLayout,
                           vk::cast<vk::buffer>(dstBuffer), regionCount,
                           pRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(VkCommandBuffer commandBuffer,
                                             VkBuffer dstBuffer,
                                             VkDeviceSize dstOffset,
                                             VkDeviceSize dataSize,
                                             const void *pData) {
  vk::CmdUpdateBuffer(vk::cast<vk::command_buffer>(commandBuffer),
                      vk::cast<vk::buffer>(dstBuffer), dstOffset, dataSize,
                      pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(VkCommandBuffer commandBuffer,
                                           VkBuffer dstBuffer,
                                           VkDeviceSize dstOffset,
                                           VkDeviceSize size, uint32_t data) {
  vk::CmdFillBuffer(vk::cast<vk::command_buffer>(commandBuffer),
                    vk::cast<vk::buffer>(dstBuffer), dstOffset, size, data);
}

VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(
    VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
    const VkClearColorValue *pColor, uint32_t rangeCount,
    const VkImageSubresourceRange *pRanges) {
  vk::CmdClearColorImage(vk::cast<vk::command_buffer>(commandBuffer),
                         vk::cast<vk::image>(image), imageLayout, pColor,
                         rangeCount, pRanges);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage,
                            VkImageLayout, const VkClearDepthStencilValue *,
                            uint32_t, const VkImageSubresourceRange *) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdClearAttachments(VkCommandBuffer commandBuffer,
                                                 uint32_t,
                                                 const VkClearAttachment *,
                                                 uint32_t,
                                                 const VkClearRect *) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(VkCommandBuffer commandBuffer,
                                             VkImage, VkImageLayout, VkImage,
                                             VkImageLayout, uint32_t,
                                             const VkImageResolve *) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(VkCommandBuffer commandBuffer,
                                         VkEvent event,
                                         VkPipelineStageFlags stageMask) {
  vk::CmdSetEvent(vk::cast<vk::command_buffer>(commandBuffer),
                  vk::cast<vk::event>(event), stageMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(VkCommandBuffer commandBuffer,
                                           VkEvent event,
                                           VkPipelineStageFlags stageMask) {
  vk::CmdResetEvent(vk::cast<vk::command_buffer>(commandBuffer),
                    vk::cast<vk::event>(event), stageMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(
    VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier *pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier *pImageMemoryBarriers) {
  vk::CmdWaitEvents(vk::cast<vk::command_buffer>(commandBuffer), eventCount,
                    pEvents, srcStageMask, dstStageMask, memoryBarrierCount,
                    pMemoryBarriers, bufferMemoryBarrierCount,
                    pBufferMemoryBarriers, imageMemoryBarrierCount,
                    pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(
    VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
    uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier *pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier *pImageMemoryBarriers) {
  vk::CmdPipelineBarrier(vk::cast<vk::command_buffer>(commandBuffer),
                         srcStageMask, dstStageMask, dependencyFlags,
                         memoryBarrierCount, pMemoryBarriers,
                         bufferMemoryBarrierCount, pBufferMemoryBarriers,
                         imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(VkCommandBuffer commandBuffer,
                                           VkQueryPool queryPool,
                                           uint32_t query,
                                           VkQueryControlFlags flags) {
  vk::CmdBeginQuery(vk::cast<vk::command_buffer>(commandBuffer),
                    vk::cast<vk::query_pool>(queryPool), query, flags);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(VkCommandBuffer commandBuffer,
                                         VkQueryPool queryPool,
                                         uint32_t query) {
  vk::CmdEndQuery(vk::cast<vk::command_buffer>(commandBuffer),
                  vk::cast<vk::query_pool>(queryPool), query);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(VkCommandBuffer commandBuffer,
                                               VkQueryPool queryPool,
                                               uint32_t firstQuery,
                                               uint32_t queryCount) {
  vk::CmdResetQueryPool(vk::cast<vk::command_buffer>(commandBuffer),
                        vk::cast<vk::query_pool>(queryPool), firstQuery,
                        queryCount);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(
    VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage,
    VkQueryPool queryPool, uint32_t query) {
  vk::CmdWriteTimestamp(vk::cast<vk::command_buffer>(commandBuffer),
                        pipelineStage, vk::cast<vk::query_pool>(queryPool),
                        query);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(
    VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
    uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset,
    VkDeviceSize stride, VkQueryResultFlags flags) {
  vk::CmdCopyQueryPoolResults(vk::cast<vk::command_buffer>(commandBuffer),
                              vk::cast<vk::query_pool>(queryPool), firstQuery,
                              queryCount, dstBuffer, dstOffset, stride, flags);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(VkCommandBuffer commandBuffer,
                                              VkPipelineLayout layout,
                                              VkShaderStageFlags stageFlags,
                                              uint32_t offset, uint32_t size,
                                              const void *pValues) {
  vk::CmdPushConstants(vk::cast<vk::command_buffer>(commandBuffer),
                       vk::cast<vk::pipeline_layout>(layout), stageFlags,
                       offset, size, pValues);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(VkCommandBuffer commandBuffer,
                                                const VkRenderPassBeginInfo *,
                                                VkSubpassContents) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass(VkCommandBuffer commandBuffer,
                                            VkSubpassContents) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(VkCommandBuffer commandBuffer) {
  // no stub since graphics operations will never be supported
  vk::cast<vk::command_buffer>(commandBuffer)->error =
      VK_ERROR_FEATURE_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL
vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
                     const VkCommandBuffer *pCommandBuffers) {
  vk::CmdExecuteCommands(vk::cast<vk::command_buffer>(commandBuffer),
                         commandBufferCount,
                         vk::cast<const vk::command_buffer *>(pCommandBuffers));
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures) {
  vk::GetPhysicalDeviceFeatures2(vk::cast<vk::physical_device>(physicalDevice),
                                 pFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 *pProperties) {
  vk::GetPhysicalDeviceProperties2(
      vk::cast<vk::physical_device>(physicalDevice), pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2(
    VkPhysicalDevice physicalDevice, VkFormat format,
    VkFormatProperties2 *pFormatProperties) {
  vk::GetPhysicalDeviceFormatProperties2(
      vk::cast<vk::physical_device>(physicalDevice), format, pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo,
    VkImageFormatProperties2 *pImageFormatProperties) {
  return vk::GetPhysicalDeviceImageFormatProperties2(
      vk::cast<vk::physical_device>(physicalDevice), pImageFormatInfo,
      pImageFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2 *pQueueFamilyProperties) {
  vk::GetPhysicalDeviceQueueFamilyProperties2(
      vk::cast<vk::physical_device>(physicalDevice), pQueueFamilyPropertyCount,
      pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties2(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2 *pMemoryProperties) {
  vk::GetPhysicalDeviceMemoryProperties2(
      vk::cast<vk::physical_device>(physicalDevice), pMemoryProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties2(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo,
    uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties) {
  vk::GetPhysicalDeviceSparseImageFormatProperties2(
      vk::cast<vk::physical_device>(physicalDevice), pFormatInfo,
      pPropertyCount, pProperties);
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *pSupportedVersion) {
  if (static_cast<int>(*pSupportedVersion) <
      MIN_SUPPORTED_LOADER_ICD_INTERFACE_VERSION) {
    return VK_ERROR_INCOMPATIBLE_DRIVER;
  }

  if (*pSupportedVersion < CURRENT_LOADER_ICD_INTERFACE_VERSION) {
    return VK_SUCCESS;
  }

  *pSupportedVersion = CURRENT_LOADER_ICD_INTERFACE_VERSION;

  return VK_SUCCESS;
}
