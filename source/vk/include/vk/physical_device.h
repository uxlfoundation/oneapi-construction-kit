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

#ifndef VK_PHYSICAL_DEVICE_H_INCLUDED
#define VK_PHYSICAL_DEVICE_H_INCLUDED

#include <compiler/info.h>
#include <mux/mux.h>
#include <vk/icd.h>
#include <vulkan/vulkan.h>

/// @brief maximum push constant size supported by the physical device
#define CA_VK_MAX_PUSH_CONSTANTS_SIZE 128

namespace vk {
/// @copydoc ::vk::instance_t
typedef struct instance_t *instance;

/// @brief internal physical device type
typedef struct physical_device_t final : icd_t<physical_device_t> {
  /// @brief constructor
  physical_device_t(vk::instance instance, mux_device_info_t device_info,
                    const compiler::Info *compiler_info,
                    VkPhysicalDeviceMemoryProperties memory_properties);

  /// @brief destructor
  ~physical_device_t();

  /// @brief Pointer to the instance this physical_device was created from.
  vk::instance instance;

  /// @brief Pointer to corresponding mux_device_info.
  mux_device_info_t device_info;

  /// @brief Pointer to corresponding compiler::Info.
  const compiler::Info *compiler_info;

  /// @brief Struct containing general information about this physical device
  VkPhysicalDeviceProperties properties;

  /// @brief Struct containing infomarmation about this device's features
  VkPhysicalDeviceFeatures features;

  /// @brief This device's memory properties
  VkPhysicalDeviceMemoryProperties memory_properties;

#if CA_VK_KHR_variable_pointers
  /// @brief Struct containing information about variable pointers features.
  VkPhysicalDeviceVariablePointerFeatures features_variable_pointers;
#endif
} *physical_device;

/// @brief Struct containing the header for an extension struct
///
/// Extension structs can be passed in the `pNext` member of various create info
/// and properties structs, each of them starts with two commons members that
/// can be used to determine the nature of the struct and get to the next struct
/// in the pNext chain.
typedef struct {
  /// @brief VK structure type enum that identifies the struct
  VkStructureType sType;
  /// @brief If not null this points to the next struct in the pNext chain
  void *pNext;
} pnext_struct_header;

/// @brief Internal implementation of vkEnumeratePhysicalDevices
///
/// @param instance Instance to query for devices.
/// @param pPhysicalDeviceCount Return number of devices if pPhysicalDevices is
/// null, otherwise this should be the length of pPhysicalDevices
/// @param pPhysicalDevices Return handles to present physical devices if not
/// null
///
/// @return Return result code.
VkResult EnumeratePhysicalDevices(vk::instance instance,
                                  uint32_t *pPhysicalDeviceCount,
                                  vk::physical_device *pPhysicalDevices);

/// @brief Internal implementation of vkGetPhysicalDeviceProperties
///
/// @param physicalDevice Physical device to query for properties
/// @param pProperties Return physical device properties
void GetPhysicalDeviceProperties(vk::physical_device physicalDevice,
                                 VkPhysicalDeviceProperties *pProperties);

/// @brief Internal implementation of vkGetPhysicalDeviceQueueFamilyProperties
///
/// @param physicalDevice The physical device to query
/// @param pQueueFamilyPropertyCount Return number of available queue families
/// if pQueueFamilyProperties is null, otherwise defines the maximum number of
/// elements in pQueueFamilyProperties that can be overwritten
/// @param pQueueFamilyProperties Return queue family properties up to a
/// maximum of pQueueFamilyPropertyCount if not null
void GetPhysicalDeviceQueueFamilyProperties(
    vk::physical_device physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties *pQueueFamilyProperties);

#if CA_VK_KHR_get_physical_device_properties2
/// @brief Internal implementation of vkGetPhysicalDeviceQueueFamilyProperties2
///
/// @param physicalDevice The physical device to query
/// @param pQueueFamilyPropertyCount Return number of available queue families
/// if pQueueFamilyProperties is null, otherwise defines the maximum number of
/// elements in pQueueFamilyProperties that can be overwritten
/// @param pQueueFamilyProperties Return queue family properties up to a
/// maximum of pQueueFamilyPropertyCount if not null
void GetPhysicalDeviceQueueFamilyProperties2(
    vk::physical_device physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2 *pQueueFamilyProperties);
#endif

/// @brief internal implementation of vkGetPhysicalDeviceMemoryProperties
///
/// @param physicalDevice the physical device to query for memory properties
/// @param pMemoryProperties return memory properties
void GetPhysicalDeviceMemoryProperties(
    vk::physical_device physicalDevice,
    VkPhysicalDeviceMemoryProperties *pMemoryProperties);

#if CA_VK_KHR_get_physical_device_properties2
/// @brief Internal implementation of vkGetPhysicalDeviceMemoryProperties2
///
/// @param physicalDevice Physical device to query for memory properties
/// @param pMemoryProperties Return physical device memory properties
void GetPhysicalDeviceMemoryProperties2(
    vk::physical_device physicalDevice,
    VkPhysicalDeviceMemoryProperties2 *pMemoryProperties);
#endif

/// @brief internal implementation of vkGetPhysicalDeviceFeatures
///
/// @param physicalDevice the physical device to query for features
/// @param pFeatures return physical device features
void GetPhysicalDeviceFeatures(vk::physical_device physicalDevice,
                               VkPhysicalDeviceFeatures *pFeatures);

#if CA_VK_KHR_get_physical_device_properties2
/// @brief Internal implementation of vkGetPhysicalDeviceFeatures2
///
/// @param physicalDevice The physical device to query for features
/// @param pFeatures Return physical device features
void GetPhysicalDeviceFeatures2(vk::physical_device physicalDevice,
                                VkPhysicalDeviceFeatures2 *pFeatures);
#endif

#if CA_VK_KHR_get_physical_device_properties2
/// @brief Internal implementation of vkGetPhysicalDeviceProperties2
///
/// @param physicalDevice The physical device to query for properties
/// @param pProperties Return physical device properties
void GetPhysicalDeviceProperties2(vk::physical_device physicalDevice,
                                  VkPhysicalDeviceProperties2 *pProperties);
#endif

/// @brief Stub of vkGetPhysicalDeviceFormatProperties
void GetPhysicalDeviceFormatProperties(vk::physical_device physicalDevice,
                                       VkFormat format,
                                       VkFormatProperties *pFormatProperties);

#if CA_VK_KHR_get_physical_device_properties2
/// @brief Stub of vkGetPhysicalDeviceFormatProperties2
void GetPhysicalDeviceFormatProperties2(vk::physical_device physicalDevice,
                                        VkFormat format,
                                        VkFormatProperties2 *pFormatProperties);
#endif

/// @brief Stub of vkGetPhysicalDeviceImageFormatProperties
VkResult GetPhysicalDeviceImageFormatProperties(
    vk::physical_device physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkImageFormatProperties *pImageFormatProperties);

#if CA_VK_KHR_get_physical_device_properties2
/// @brief Stub of vkGetPhysicalDeviceImageFormatProperties2
VkResult GetPhysicalDeviceImageFormatProperties2(
    vk::physical_device physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo,
    VkImageFormatProperties2 *pImageFormatProperties);
#endif

/// @brief Stub of vkGetPhysicalDeviceSparseImageFormatProperties
void GetPhysicalDeviceSparseImageFormatProperties(
    vk::physical_device physicalDevice, VkFormat format, VkImageType type,
    VkSampleCountFlagBits samples, VkImageUsageFlags usage,
    VkImageTiling tiling, uint32_t *pPropertyCount,
    VkSparseImageFormatProperties *pProperties);

#if CA_VK_KHR_get_physical_device_properties2
/// @brief Stub of vkGetPhysicalDeviceSparseImageProperties2
void GetPhysicalDeviceSparseImageFormatProperties2(
    vk::physical_device physicalDevice,
    const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo,
    uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties);
#endif
}  // namespace vk

#endif  // VK_PHYSICAL_DEVICE_H_INCLUDED
