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

#include <vk/instance.h>
#include <vk/physical_device.h>

#include <string>

namespace vk {
instance_t::instance_t(const VkInstanceCreateInfo *pCreateInfo,
                       vk::allocator allocator)
    : pCreateInfo(*pCreateInfo),
      allocator(allocator),
      devices({allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

instance_t::~instance_t() {}

VkResult CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                        vk::allocator allocator, vk::instance *pInstance) {
  if (pCreateInfo->pApplicationInfo) {
    // if apiVersion was left as zero it is to be ignored
    if (pCreateInfo->pApplicationInfo->apiVersion) {
      if (1 != VK_VERSION_MAJOR(pCreateInfo->pApplicationInfo->apiVersion) ||
          0 != VK_VERSION_MINOR(pCreateInfo->pApplicationInfo->apiVersion)) {
        return VK_ERROR_INCOMPATIBLE_DRIVER;
      }
    }
  }

  if (0 < pCreateInfo->enabledLayerCount) {
    // We do not currently support any driver internal layers.
    return VK_ERROR_LAYER_NOT_PRESENT;
  }

  // verify any requested extensions are supported (i.e. present in our list)
  for (uint32_t extIndex = 0; extIndex < pCreateInfo->enabledExtensionCount;
       extIndex++) {
    const std::string extensionName =
        pCreateInfo->ppEnabledExtensionNames[extIndex];

    const auto findExtension =
        [&extensionName](const VkExtensionProperties &extension) -> bool {
      return extensionName == std::string(extension.extensionName);
    };

    if (std::find_if(vk::instance_extensions.begin(),
                     vk::instance_extensions.end(),
                     findExtension) == vk::instance_extensions.end()) {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }

  vk::instance instance = allocator.create<vk::instance_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, pCreateInfo, allocator);
  if (!instance) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  uint64_t muxDeviceCount;
  vk::small_vector<mux_device_info_t, 2> muxDevices(
      {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});

  // if we don't store a reference to allocator in instance and pass that
  // instead of the one we get as a parameter here, some funky loader stuff
  // happens which messes with the user_data in muxAllocator and causes a crash
  // when we cast it back to an allocator and try to use it
  mux_result_t error =
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &muxDeviceCount);

  if (error) {
    return vk::getVkResult(error);
  }

  if (muxDevices.resize(muxDeviceCount)) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  error = muxGetDeviceInfos(mux_device_type_all, muxDeviceCount,
                            muxDevices.data(), nullptr);

  if (error) {
    return vk::getVkResult(error);
  }

  for (mux_device_info_t device_info : muxDevices) {
    // VK does not support logical only, the device must report 32 or 64 to be
    // considered valid
    if (device_info->address_capabilities &
        (mux_address_capabilities_bits32 | mux_address_capabilities_bits64)) {
      VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
      const VkMemoryHeap heap = {device_info->memory_size,
                                 VK_MEMORY_HEAP_DEVICE_LOCAL_BIT};

      deviceMemoryProperties.memoryHeapCount = 1;
      deviceMemoryProperties.memoryHeaps[0] = heap;

      vk::small_vector<VkMemoryType, 3> types({
          allocator.getCallbacks(),
          VK_SYSTEM_ALLOCATION_SCOPE_COMMAND,
      });

      if (device_info->allocation_capabilities &
          mux_allocation_capabilities_coherent_host) {
        if (types.push_back({VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             0})) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }

      if (device_info->allocation_capabilities &
          mux_allocation_capabilities_cached_host) {
        if (types.push_back({VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
                             0})) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }

      if (device_info->allocation_capabilities &
          mux_allocation_capabilities_alloc_device) {
        if (types.push_back({VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0})) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }

      deviceMemoryProperties.memoryTypeCount = types.size();

      for (size_t typeIndex = 0; typeIndex < types.size(); typeIndex++) {
        deviceMemoryProperties.memoryTypes[typeIndex] = types[typeIndex];
      }

      // The Mux device must have a compiler associated with it.
      const compiler::Info *compiler_info =
          compiler::getCompilerForDevice(device_info);
      if (!compiler_info) {
        return VK_ERROR_INITIALIZATION_FAILED;
      }

      if (instance->devices.push_back(allocator.create<vk::physical_device_t>(
              VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, instance, device_info,
              compiler_info, deviceMemoryProperties))) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
  }

  *pInstance = instance;

  return VK_SUCCESS;
}

void DestroyInstance(vk::instance instance, const vk::allocator allocator) {
  if (instance == VK_NULL_HANDLE) {
    return;
  }

  for (vk::physical_device device : instance->devices) {
    allocator.destroy(device);
  }
  allocator.destroy(instance);
}

VkResult EnumerateInstanceExtensionProperties(
    const char *, uint32_t *pPropertyCount,
    VkExtensionProperties *pProperties) {
  if (pProperties) {
    std::copy(vk::instance_extensions.begin(),
              std::min(vk::instance_extensions.begin() + *pPropertyCount,
                       vk::instance_extensions.end()),
              pProperties);

  } else {
    *pPropertyCount = vk::instance_extensions.size();
  }
  return VK_SUCCESS;
}
}  // namespace vk
