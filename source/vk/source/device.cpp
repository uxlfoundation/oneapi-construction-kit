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

#include <builtins/bakery.h>
#include <cargo/array_view.h>
#include <cargo/string_view.h>
#include <vk/device.h>
#include <vk/instance.h>
#include <vk/physical_device.h>
#include <vk/queue.h>
#include <vk/unique_ptr.h>

#ifndef NDEBUG
#include <compiler/utils/llvm_global_mutex.h>
#include <llvm/Support/CommandLine.h>
namespace {
std::once_flag parseEnvironmentOptionsFlag;
}  // namespace
#endif

#include <utility>

namespace vk {
device_t::device_t(vk::allocator allocator,
                   mux::unique_ptr<mux_device_t> mux_device,
                   VkPhysicalDeviceMemoryProperties *memory_properties,
                   VkPhysicalDeviceProperties *physical_device_properties,
                   std::unique_ptr<compiler::Target> compiler_target,
                   std::unique_ptr<compiler::Context> compiler_context,
                   compiler::spirv::DeviceInfo spv_device_info)
    : allocator(allocator),
      mux_device(mux_device.release()),
      memory_properties(*memory_properties),
      physical_device_properties(*physical_device_properties),
      compiler_target(std::move(compiler_target)),
      compiler_context(std::move(compiler_context)),
      spv_device_info(std::move(spv_device_info)) {}

device_t::~device_t() {
  // In accordance with the spec, queues are created and destroyed along with
  // their devices
  compiler_target.reset();
  if (queue) {
    allocator.destroy(queue);
  }
  muxDestroyDevice(mux_device, allocator.getMuxAllocator());
}

VkResult CreateDevice(vk::physical_device physicalDevice,
                      const VkDeviceCreateInfo *pCreateInfo,
                      vk::allocator allocator, vk::device *pDevice) {
  // TODO: Support creation of multiple queues across multiple queue families
  for (uint32_t queueCreateInfoIndex = 0,
                queueCreateInfoEnd = pCreateInfo->queueCreateInfoCount;
       queueCreateInfoIndex < queueCreateInfoEnd; queueCreateInfoIndex++) {
    if (pCreateInfo->pQueueCreateInfos[queueCreateInfoIndex].queueFamilyIndex !=
            0 ||
        pCreateInfo->pQueueCreateInfos[queueCreateInfoIndex].queueCount != 1) {
      return VK_ERROR_INITIALIZATION_FAILED;
    }
  }

  auto isExtensionPresent = [](cargo::string_view extensionName) {
    auto found = std::find_if(
        vk::device_extensions.begin(), vk::device_extensions.end(),
        [&extensionName](const VkExtensionProperties &extension) {
          return extensionName.compare(extension.extensionName) == 0;
        });
    return found != vk::device_extensions.end();
  };

  // verify any requested extensions are supported (i.e. present in our list)
  for (uint32_t extIndex = 0; extIndex < pCreateInfo->enabledExtensionCount;
       extIndex++) {
    if (!isExtensionPresent(pCreateInfo->ppEnabledExtensionNames[extIndex])) {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }

  auto isExtensionEnabled = [&](cargo::string_view extensionName) {
    auto enabledExtensions = cargo::array_view<const char *const>(
        pCreateInfo->ppEnabledExtensionNames,
        pCreateInfo->ppEnabledExtensionNames +
            pCreateInfo->enabledExtensionCount);
    auto found =
        std::find_if(enabledExtensions.begin(), enabledExtensions.end(),
                     [&](const char *const enabledExtensionName) {
                       return extensionName.compare(enabledExtensionName) == 0;
                     });
    return found != enabledExtensions.end();
  };
  (void)isExtensionEnabled;  // Maybe unused if all extensions are disabled.

  // this implementation does not provide any layers
  if (pCreateInfo->enabledLayerCount) {
    return VK_ERROR_LAYER_NOT_PRESENT;
  }

  // Populate spvDeviceInfo from VKDevice.
  compiler::spirv::DeviceInfo spvDeviceInfo;
  if (cargo::success !=
      spvDeviceInfo.capabilities.assign({
          spv::CapabilityMatrix,  // TODO(CA-341): Implement matrix support.
          spv::CapabilityShader,
          spv::CapabilityInputAttachment,
          spv::CapabilitySampled1D,
          spv::CapabilityImage1D,
          spv::CapabilitySampledBuffer,
          spv::CapabilityImageBuffer,
          spv::CapabilityImageQuery,
          spv::CapabilityDerivativeControl,
      })) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }
  if (physicalDevice->features.shaderFloat64) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(spv::CapabilityFloat64)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderInt64) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(spv::CapabilityInt64)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
#if CA_VK_KHR_shader_atomic_int64
  if (isExtensionEnabled("VK_KHR_shader_atomic_int64")) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(spv::CapabilityInt64Atomics)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
#endif
  if (physicalDevice->features.shaderInt16) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(spv::CapabilityInt16)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderImageGatherExtended) {
    if (cargo::success != spvDeviceInfo.capabilities.push_back(
                              spv::CapabilityImageGatherExtended)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderStorageImageMultisample) {
    if (cargo::success != spvDeviceInfo.capabilities.push_back(
                              spv::CapabilityStorageImageMultisample)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderUniformBufferArrayDynamicIndexing) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(
            spv::CapabilityUniformBufferArrayDynamicIndexing)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderSampledImageArrayDynamicIndexing) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(
            spv::CapabilitySampledImageArrayDynamicIndexing)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderStorageBufferArrayDynamicIndexing) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(
            spv::CapabilityStorageBufferArrayDynamicIndexing)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderStorageImageArrayDynamicIndexing) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(
            spv::CapabilityStorageImageArrayDynamicIndexing)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.imageCubeArray) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(spv::CapabilityImageCubeArray)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderResourceResidency) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(spv::CapabilitySparseResidency)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderResourceMinLod) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(spv::CapabilityMinLod)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.imageCubeArray) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(spv::CapabilitySampledCubeArray)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderStorageImageMultisample) {
    if (cargo::success !=
        spvDeviceInfo.capabilities.push_back(spv::CapabilityImageMSArray)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderStorageImageExtendedFormats) {
    if (cargo::success != spvDeviceInfo.capabilities.push_back(
                              spv::CapabilityStorageImageExtendedFormats)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderStorageImageReadWithoutFormat) {
    if (cargo::success != spvDeviceInfo.capabilities.push_back(
                              spv::CapabilityStorageImageReadWithoutFormat)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
  if (physicalDevice->features.shaderStorageImageWriteWithoutFormat) {
    if (cargo::success != spvDeviceInfo.capabilities.push_back(
                              spv::CapabilityStorageImageWriteWithoutFormat)) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
#if CA_VK_KHR_variable_pointers
  if (isExtensionEnabled("VK_KHR_variable_pointers")) {
    if (physicalDevice->features_variable_pointers
            .variablePointersStorageBuffer) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilityVariablePointersStorageBuffer)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->features_variable_pointers.variablePointers) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilityVariablePointers)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (cargo::success !=
        spvDeviceInfo.extensions.push_back("SPV_KHR_variable_pointers")) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
#endif
#if CA_VK_KHR_16bit_storage
  if (isExtensionEnabled("VK_KHR_16bit_storage")) {
    if (physicalDevice->features_16bit_storage.storageBuffer16BitAccess) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilityStorageBuffer16BitAccess)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->features_16bit_storage
            .uniformAndStorageBuffer16BitAccess) {
      if (cargo::success !=
          spvDeviceInfo.capabilities.push_back(
              spv::CapabilityUniformAndStorageBuffer16BitAccess)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->features_16bit_storage.storagePushConstant16) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilityStoragePushConstant16)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->features_16bit_storage.storageInputOutput16) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilityStorageInputOutput16)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (cargo::success !=
        spvDeviceInfo.extensions.push_back("SPV_KHR_16bit_storage")) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
#endif
#if CA_VK_KHR_shader_float16_int8
  if (isExtensionEnabled("VK_KHR_shader_float16_int8")) {
    if (physicalDevice->features_shader_float16_int8.shaderFloat16) {
      if (cargo::success !=
          spvDeviceInfo.capabilities.push_back(spv::CapabilityFloat16)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->features_shader_float16_int8.shaderInt8) {
      if (cargo::success !=
          spvDeviceInfo.capabilities.push_back(spv::CapabilityInt8)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
  }
#endif
#if CA_VK_KHR_8bit_storage
  if (isExtensionEnabled("VK_KHR_8bit_storage")) {
    if (physicalDevice->features_8bit_storage.StorageBuffer8BitAccess) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilityStorageBuffer8BitAccess)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->features_8bit_storage
            .UniformAndStorageBuffer8BitAccess) {
      if (cargo::success !=
          spvDeviceInfo.capabilities.push_back(
              spv::CapabilityUniformAndStorageBuffer8BitAccess)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->features_8bit_storage.StoragePushConstant8) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilityStoragePushConstant8)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
  }
#endif
#if CA_VK_KHR_vulkan_memory_model
  if (isExtensionEnabled("VK_KHR_vulkan_memory_model")) {
    if (physicalDevice->features_vulkan_memory_model.vulkanMemoryModel) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilityVulkanMemoryModelKHR)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->features_vulkan_memory_model
            .vulkanMemoryModelDeviceScope) {
      if (cargo::success !=
          spvDeviceInfo.capabilities.push_back(
              spv::CapabilityVulkanMemoryModelDeviceScopeKHR)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (cargo::success !=
        spvDeviceInfo.extensions.push_back("SPV_KHR_vulkan_memory_model")) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
#endif
#if CA_VK_KHR_shader_float_controls
  if (isExtensionEnabled("VK_KHR_shader_float_controls")) {
    if (physicalDevice->properties_shader_float_controls
            .shaderDenormPreserveFloat16 &&
        physicalDevice->properties_shader_float_controls
            .shaderDenormPreserveFloat32 &&
        physicalDevice->properties_shader_float_controls
            .shaderDenormPreserveFloat64) {
      if (cargo::success !=
          spvDeviceInfo.capabilities.push_back(spv::CapabilityDenormPreserve)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->properties_shader_float_controls
            .shaderDenormFlushToZeroFloat16 &&
        physicalDevice->properties_shader_float_controls
            .shaderDenormFlushToZeroFloat32 &&
        physicalDevice->properties_shader_float_controls
            .shaderDenormFlushToZeroFloat64) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilityDenormFlushToZero)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->properties_shader_float_controls
            .shaderSignedZeroInfNanPreserveFloat16 &&
        physicalDevice->properties_shader_float_controls
            .shaderSignedZeroInfNanPreserveFloat32 &&
        physicalDevice->properties_shader_float_controls
            .shaderSignedZeroInfNanPreserveFloat64) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilitySignedZeroInfNanPreserve)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->properties_shader_float_controls
            .shaderRoundingModeRTEFloat16 &&
        physicalDevice->properties_shader_float_controls
            .shaderRoundingModeRTEFloat32 &&
        physicalDevice->properties_shader_float_controls
            .shaderRoundingModeRTEFloat64) {
      if (cargo::success != spvDeviceInfo.capabilities.push_back(
                                spv::CapabilityRoundingModeRTE)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    if (physicalDevice->properties_shader_float_controls
            .shaderRoundingModeRTZFloat16 &&
        physicalDevice->properties_shader_float_controls
            .shaderRoundingModeRTZFloat32 &&
        physicalDevice->properties_shader_float_controls
            .shaderRoundingModeRTZFloat64) {
      if (cargo::success !=
          spvDeviceInfo.capabilities.push_back(spv::CapabilityRoundingModeRTZ))) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }
    }
    if (cargo::success !=
        spvDeviceInfo.extensions.push_back("SPV_KHR_float_controls")) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
#endif
#if CA_VK_KHR_storage_buffer_storage_class
  if (isExtensionEnabled("VK_KHR_storage_buffer_storage_class")) {
    if (cargo::success != spvDeviceInfo.extensions.push_back(
                              "SPV_KHR_storage_buffer_storage_class")) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }
#endif

  // This extension is always supported, it's basically just a compiler hint.
  if (cargo::success != spvDeviceInfo.extensions.push_back(
                            "SPV_KHR_no_integer_wrap_decoration")) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  if (cargo::success !=
      spvDeviceInfo.ext_inst_imports.push_back("GLSL.std.450")) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }
  spvDeviceInfo.addressing_model = spv::AddressingModelLogical;
  spvDeviceInfo.memory_model = spv::MemoryModelGLSL450;

  uint32_t caps = 0;

  // deduce whether device has 32 or 64 bit addressing
  if (physicalDevice->device_info->address_capabilities &
      mux_address_capabilities_bits32) {
    caps |= compiler::CAPS_32BIT;
    spvDeviceInfo.address_bits = 32;
  } else if (physicalDevice->device_info->address_capabilities &
             mux_address_capabilities_bits64) {
    spvDeviceInfo.address_bits = 64;
  }

  // deduce whether device meets all the requirements for doubles
  // TODO: CA-882 VK requires very few things of doubles, so any of the
  // capabilities will do for now.
  if (physicalDevice->device_info->double_capabilities) {
    caps |= compiler::CAPS_FP64;
  }
  // TODO: CA-882 It's not clear which capabilities are required for VK half
  // TODO: CA-667 Enable halfs when ready
  if (physicalDevice->device_info->half_capabilities) {
    // CA-1084: Currently we pay attention to whether the device supports FP16
    // because we need to load the builtins library that was built according to
    // the Mux device properties.  However, Vulkan doesn't need FP16 and thus
    // a smaller library could be built as well.
    caps |= compiler::CAPS_FP16;
  }

  mux_device_t mux_device;
  mux_result_t error =
      muxCreateDevices(1, &physicalDevice->device_info,
                       allocator.getMuxAllocator(), &mux_device);
  if (mux_success != error) {
    return vk::getVkResult(error);
  }
  mux::unique_ptr<mux_device_t> mux_device_ptr(mux_device,
                                               allocator.getMuxAllocator());

  // Initialise compiler target and context.
  std::unique_ptr<compiler::Context> compiler_context =
      compiler::createContext();
  if (!compiler_context) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  std::unique_ptr<compiler::Target> compiler_target =
      physicalDevice->compiler_info->createTarget(compiler_context.get(),
                                                  nullptr);
  if (!compiler_target ||
      compiler_target->init(caps) != compiler::Result::SUCCESS) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  vk::device device = allocator.create<vk::device_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, allocator, std::move(mux_device_ptr),
      &physicalDevice->memory_properties, &physicalDevice->properties,
      std::move(compiler_target), std::move(compiler_context),
      std::move(spvDeviceInfo));

  if (!device) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  vk::unique_ptr<vk::device> device_ptr(device, allocator);

  mux_queue_t mux_queue;

  error = muxGetQueue(device_ptr->mux_device, mux_queue_type_compute, 0,
                      &mux_queue);

  if (mux_success != error) {
    return vk::getVkResult(error);
  }

  device_ptr->queue = allocator.create<vk::queue_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, mux_queue, allocator);

  if (!device_ptr->queue) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  mux_command_buffer_t fence_command_buffer;
  error = muxCreateCommandBuffer(device_ptr->mux_device, nullptr,
                                 allocator.getMuxAllocator(),
                                 &fence_command_buffer);

  if (mux_success != error) {
    return vk::getVkResult(error);
  }

  device_ptr->queue->fence_command_buffer = fence_command_buffer;

  *pDevice = device_ptr.release();

#ifndef NDEBUG
  std::call_once(parseEnvironmentOptionsFlag, []() {
    const char *argv[] = {"ComputeAortaVK"};
    const std::lock_guard<std::mutex> lock(
        compiler::utils::getLLVMGlobalMutex());
    llvm::cl::ParseCommandLineOptions(1, argv, "", nullptr, "CA_LLVM_OPTIONS");
  });
#endif

  return VK_SUCCESS;
}

void DestroyDevice(vk::device device, const vk::allocator allocator) {
  if (device == VK_NULL_HANDLE) {
    return;
  }

  allocator.destroy(device);
}

VkResult DeviceWaitIdle(vk::device device) {
  // TODO: when/if we support multiple queues make this wait for each queue
  const mux_result_t error = muxWaitAll(device->queue->mux_queue);
  if (mux_success != error) {
    return vk::getVkResult(error);
  }

  return VK_SUCCESS;
}

void GetDeviceMemoryCommitment(vk::device device, vk::device_memory memory,
                               VkDeviceSize *pCommittedMemoryInBytes) {
  (void)device;
  (void)memory;
  (void)pCommittedMemoryInBytes;
}

VkResult EnumerateDeviceExtensionProperties(
    vk::physical_device, const char *, uint32_t *pPropertyCount,
    VkExtensionProperties *pProperties) {
  if (pProperties) {
    std::copy(vk::device_extensions.begin(),
              std::min(vk::device_extensions.begin() + *pPropertyCount,
                       vk::device_extensions.end()),
              pProperties);
  } else {
    *pPropertyCount = vk::device_extensions.size();
  }
  return VK_SUCCESS;
}
}  // namespace vk
