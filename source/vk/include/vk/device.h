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

#ifndef VK_DEVICE_H_INCLUDED
#define VK_DEVICE_H_INCLUDED

#include <compiler/context.h>
#include <compiler/module.h>
#include <compiler/target.h>
#include <mux/mux.hpp>
#include <vk/allocator.h>
#include <vk/error.h>
#include <vk/icd.h>

#include <array>

namespace vk {
/// @copydoc ::vk::physical_device_t
typedef struct physical_device_t *physical_device;

/// @copydoc ::vk::queue_t
typedef struct queue_t *queue;

/// @copydoc ::vk::device_memory_t
typedef struct device_memory_t *device_memory;

/// @brief Internal device type
typedef struct device_t final : icd_t<device_t> {
  /// @brief Constructor
  ///
  /// @param allocator Allocator for creation of objects in functions that don't
  /// get an allocator through parameters
  /// @param mux_device Instance of mux_device_t to take ownership of
  /// @param memory_properties Pointer to memory properties struct from the
  /// physical device this device is based on
  /// @param physical_device_properties Pointer to device properties struct from
  /// the physical device this device is based on
  /// @param compiler_target Compiler target that was created alongside the
  /// device
  /// @param compiler_context Compiler context that was created alongside the
  /// device
  /// @param spv_device_info SPIR-V device info to pass to compiler
  device_t(vk::allocator allocator, mux::unique_ptr<mux_device_t> mux_device,
           VkPhysicalDeviceMemoryProperties *memory_properties,
           VkPhysicalDeviceProperties *physical_device_properties,
           std::unique_ptr<compiler::Target> compiler_target,
           std::unique_ptr<compiler::Context> compiler_context,
           compiler::spirv::DeviceInfo spv_device_info);

  /// @brief Destructor
  ~device_t();

  /// @brief Allocator for use where an allocator can't otherwise be accessed
  vk::allocator allocator;

  /// @brief Mux device
  mux_device_t mux_device;

  /// @brief queue that can be retrieved with GetDeviceQueue
  vk::queue queue;

  /// @brief This device's memory properties
  const VkPhysicalDeviceMemoryProperties &memory_properties;

  /// @brief Pointer to the underlying device's properties struct
  const VkPhysicalDeviceProperties &physical_device_properties;

  /// @brief the compiler target that will be used for kernel creation with this
  /// device
  std::unique_ptr<compiler::Target> compiler_target;

  /// @brief the compiler context that will be used as part of kernel creation.
  std::unique_ptr<compiler::Context> compiler_context;

  /// @brief Information about the device used during SPIR-V consumption.
  const compiler::spirv::DeviceInfo spv_device_info;
} *device;

/// @brief The master list of device extensions this implementation implements
static constexpr std::array<VkExtensionProperties, 3> device_extensions = {{
    {"VK_KHR_storage_buffer_storage_class", 1},
    {"VK_KHR_variable_pointers", 1},
}};

/// @brief Internal implementation of vkCreateDevice
///
/// @param physicalDevice The physical device from which the device will be
/// created
/// @param pCreateInfo Device create info
/// @param allocator Allocator
/// @param pDevice Return created device
///
/// @return Return Vulkan result code
VkResult CreateDevice(vk::physical_device physicalDevice,
                      const VkDeviceCreateInfo *pCreateInfo,
                      vk::allocator allocator, vk::device *pDevice);

/// @brief Internal implementation of vkDestroyDevice
///
/// @param device Device to be destroyed
/// @param allocator Allocator
void DestroyDevice(vk::device device, const vk::allocator allocator);

/// @brief Internal implementation of vkDeviceWaitIdle
///
/// @param device the device to idle
///
/// @return Vulkan result code
VkResult DeviceWaitIdle(vk::device device);

/// @brief Stub of vkGetDeviceMemoryCommitment
void GetDeviceMemoryCommitment(vk::device device, vk::device_memory memory,
                               VkDeviceSize *pCommittedMemoryInBytes);

/// @brief Internal implementation of vkEnumerateDeviceExtensionProperties
///
/// @param pPropertyCount Return the number of supported extensions if
/// `pProperties` is null, otherwise specifies size of `pProperties`.
/// @param pProperties Return `pPropertyCount` extension info structs.
///
/// @retrun Return result code.
VkResult EnumerateDeviceExtensionProperties(vk::physical_device, const char *,
                                            uint32_t *pPropertyCount,
                                            VkExtensionProperties *pProperties);
}  // namespace vk

#endif  // VK_DEVICE_H_INCLUDED
