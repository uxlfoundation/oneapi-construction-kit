// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vk/device.h>
#include <vk/shader_module.h>

namespace {
/// @brief Returns a 16-bit checksum of the given binary
///
/// @param code Pointer to buffer containing the module binary
/// @param code_size Size in bytes of the module binary
///
/// @return 16-bit checksum
uint32_t getModuleChecksum(const void *code, size_t code_size) {
  const char *module_buffer = reinterpret_cast<const char *>(code);

  uint32_t checksum = 2166136261;

  for (size_t char_index = 0; char_index < code_size; char_index++) {
    checksum = (checksum * 16777619) ^ module_buffer[char_index];
  }

  return checksum;
}
}  // namespace

namespace vk {
shader_module_t::shader_module_t(vk::small_vector<uint32_t, 4> code,
                                 size_t code_size, uint32_t checksum)
    : code_buffer(std::move(code)),
      code_size(code_size),
      module_checksum(checksum) {}

shader_module_t::~shader_module_t() {}

VkResult CreateShaderModule(vk::device device,
                            const VkShaderModuleCreateInfo *pCreateInfo,
                            vk::allocator allocator,
                            vk::shader_module *pShaderModule) {
  (void)device;

  vk::small_vector<uint32_t, 4> code(
      {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT});
  if (code.assign(pCreateInfo->pCode,
                  pCreateInfo->pCode + pCreateInfo->codeSize / 4)) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  uint32_t checksum =
      getModuleChecksum(pCreateInfo->pCode, pCreateInfo->codeSize);

  vk::shader_module shader_module = allocator.create<vk::shader_module_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, std::move(code),
      pCreateInfo->codeSize, checksum);

  if (!shader_module) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  *pShaderModule = shader_module;

  return VK_SUCCESS;
}

void DestroyShaderModule(vk::device device, vk::shader_module shaderModule,
                         vk::allocator allocator) {
  if (shaderModule == VK_NULL_HANDLE) {
    return;
  }

  (void)device;
  allocator.destroy(shaderModule);
}
}  // namespace vk
