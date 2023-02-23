// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_SHADER_MODULE_H_INCLUDED
#define VK_SHADER_MODULE_H_INCLUDED

#include <vk/allocator.h>
#include <vk/small_vector.h>

#include <array>
#include <cstring>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @brief internal shader_module type
typedef struct shader_module_t {
  /// @brief constructor
  ///
  /// @param code Vector contianing module binary code.
  /// @param code_size Size, in bytes, of the module binary.
  /// @param checksum Checksum of the module binary.
  shader_module_t(vk::small_vector<uint32_t, 4> code, size_t code_size,
                  uint32_t checksum);

  /// @brief destructor
  ~shader_module_t();

  /// @brief Vector containing the module binary
  vk::small_vector<uint32_t, 4> code_buffer;

  /// @brief size in bytes of the module binary
  const size_t code_size;

  /// @brief Checksum of the module binary, used to find pipeline cache entries
  const uint32_t module_checksum;
} * shader_module;

/// @brief internal implementation of vkCreateShaderModule
///
/// @param device Device on which to create the shader module
/// @param pCreateInfo create info
/// @param allocator allocator with which to create the shader module
/// @param pShaderModule return created shader module
///
/// @return return Vulkan result code
VkResult CreateShaderModule(vk::device device,
                            const VkShaderModuleCreateInfo *pCreateInfo,
                            vk::allocator allocator,
                            vk::shader_module *pShaderModule);

/// @brief internal implementation of vkDestroyShaderModule
///
/// @param device Device the shader module was created with
/// @param shaderModule shader module to be destroyed
/// @param allocator allocator used to create the shader module
void DestroyShaderModule(vk::device device, vk::shader_module shaderModule,
                         vk::allocator allocator);
}  // namespace vk

#endif  // VK_SHADER_MODULE_H_INCLUDED
