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

#include <vk/device.h>
#include <vk/pipeline_cache.h>
#include <vk/type_traits.h>

namespace vk {
cached_shader &cached_shader::operator=(cached_shader &&other) {
  binary = std::move(other.binary);
  source_checksum = other.source_checksum;
  other.source_checksum = 0;
  workgroup_size = std::move(other.workgroup_size);
  descriptor_bindings = std::move(other.descriptor_bindings);
  return *this;
}

cargo::error_or<cached_shader> cached_shader::clone() const {
  auto allocator = binary.get_allocator();
  cached_shader clone(allocator.getAllocationCallbacks(),
                      allocator.getAllocationScope());
  if (auto clone_binary = binary.clone()) {
    clone.binary = std::move(*clone_binary);
  } else {
    return clone_binary.error();
  }
  clone.source_checksum = source_checksum;
  clone.workgroup_size = workgroup_size;
  if (auto clone_descriptor_bindings = descriptor_bindings.clone()) {
    clone.descriptor_bindings = std::move(*clone_descriptor_bindings);
  } else {
    return clone_descriptor_bindings.error();
  }
  return clone;
}

bool cached_shader::operator==(const cached_shader &other) const {
  return source_checksum == other.source_checksum;
}

bool cached_shader::operator==(const uint32_t &checksum) const {
  return source_checksum == checksum;
}

pipeline_cache_t::pipeline_cache_t(vk::allocator allocator)
    : cache_entries(
          {allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT}) {}

VkResult CreatePipelineCache(vk::device device,
                             const VkPipelineCacheCreateInfo *pCreateInfo,
                             vk::allocator allocator,
                             vk::pipeline_cache *pPipelineCache) {
  vk::pipeline_cache pipeline_cache = allocator.create<pipeline_cache_t>(
      VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE, allocator);

  if (!pipeline_cache) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  if (pCreateInfo->initialDataSize) {
    const size_t header_size =
        static_cast<const uint32_t *>(pCreateInfo->pInitialData)[0];

    const uint32_t *cache_header =
        static_cast<const uint32_t *>(pCreateInfo->pInitialData);

    enum { HEADER_VERSION = 1, HEADER_VENDOR_ID = 2, HEADER_DEVICE_ID = 3 };

    if (cache_header[HEADER_VERSION] == VK_PIPELINE_CACHE_HEADER_VERSION_ONE &&
        cache_header[HEADER_VENDOR_ID] ==
            device->physical_device_properties.vendorID &&
        cache_header[HEADER_DEVICE_ID] ==
            device->physical_device_properties.deviceID) {
      const uint8_t *initialData =
          static_cast<const uint8_t *>(pCreateInfo->pInitialData) + header_size;
      size_t bytes_read = 0;

      const size_t shader_count =
          *reinterpret_cast<const size_t *>(initialData);
      bytes_read += sizeof(size_t);

      for (size_t shader_index = 0; shader_index < shader_count;
           shader_index++) {
        cached_shader shader(allocator.getCallbacks(),
                             VK_SYSTEM_ALLOCATION_SCOPE_CACHE);

        std::memcpy(&shader.data_size, initialData + bytes_read,
                    sizeof(shader.data_size));
        bytes_read += sizeof(shader.data_size);

        std::memcpy(&shader.source_checksum, initialData + bytes_read,
                    sizeof(shader.source_checksum));
        bytes_read += sizeof(shader.source_checksum);

        std::memcpy(shader.workgroup_size.data(), initialData + bytes_read,
                    sizeof(shader.workgroup_size));
        bytes_read += sizeof(shader.workgroup_size);

        size_t binary_size = 0;
        std::memcpy(&binary_size, initialData + bytes_read, sizeof(size_t));
        bytes_read += sizeof(size_t);

        if (cargo::success != shader.binary.resize(binary_size)) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        std::memcpy(shader.binary.data(), initialData + bytes_read,
                    binary_size);
        bytes_read += binary_size;

        size_t bindings_size = 0;
        std::memcpy(&bindings_size, initialData + bytes_read, sizeof(size_t));
        bytes_read += sizeof(size_t);

        if (bindings_size) {
          if (cargo::success !=
              shader.descriptor_bindings.resize(
                  bindings_size / sizeof(compiler::spirv::DescriptorBinding))) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
          }

          std::memcpy(shader.descriptor_bindings.data(),
                      initialData + bytes_read, bindings_size);
          bytes_read += bindings_size;
        }

        if (pipeline_cache->cache_entries.push_back(std::move(shader))) {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }
    }
  }

  *pPipelineCache = pipeline_cache;

  return VK_SUCCESS;
}

VkResult MergePipelineCaches(vk::device device, vk::pipeline_cache dstCache,
                             uint32_t srcCacheCount,
                             const VkPipelineCache *pSrcCaches) {
  vk::small_vector<vk::pipeline_cache, 2> src_caches(
      {device->allocator.getCallbacks(), VK_SYSTEM_ALLOCATION_SCOPE_COMMAND});

  for (uint32_t cacheIndex = 0; cacheIndex < srcCacheCount; cacheIndex++) {
    if (src_caches.push_back(
            vk::cast<vk::pipeline_cache>(pSrcCaches[cacheIndex]))) {
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }

  for (uint32_t cacheIndex = 0; cacheIndex < srcCacheCount; cacheIndex++) {
    for (const cached_shader &cachedShader :
         src_caches[cacheIndex]->cache_entries) {
      if (std::find(dstCache->cache_entries.begin(),
                    dstCache->cache_entries.end(),
                    cachedShader) == dstCache->cache_entries.end()) {
        if (auto clonedCachedShader = cachedShader.clone()) {
          if (dstCache->cache_entries.push_back(
                  std::move(*clonedCachedShader))) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
          }
        } else {
          return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }
    }
  }

  return VK_SUCCESS;
}

VkResult GetPipelineCacheData(vk::device device,
                              vk::pipeline_cache pipelineCache,
                              size_t *pDataSize, void *pData) {
  const uint32_t header_size = 16 + VK_UUID_SIZE;

  if (!pData) {
    size_t data_size = header_size;

    for (const auto &cache_entry : pipelineCache->cache_entries) {
      data_size += cache_entry.data_size;
    }

    // account for the shader count value
    data_size += sizeof(size_t);

    *pDataSize = data_size;
    return VK_SUCCESS;
  }

  uint8_t *cache_buffer = reinterpret_cast<uint8_t *>(pData);

  uint32_t header[4] = {header_size, VK_PIPELINE_CACHE_HEADER_VERSION_ONE,
                        device->physical_device_properties.vendorID,
                        device->physical_device_properties.deviceID};

  std::memcpy(cache_buffer, header, sizeof(header));

  std::memcpy(cache_buffer + sizeof(header),
              device->physical_device_properties.pipelineCacheUUID,
              VK_UUID_SIZE);

  size_t bytes_written = header_size;

  size_t *shaders_written =
      reinterpret_cast<size_t *>(cache_buffer + bytes_written);
  *shaders_written = 0;
  bytes_written += sizeof(size_t);

  for (cached_shader &cachedShader : pipelineCache->cache_entries) {
    // make sure we don't go over the allocation we've been given, only run this
    // check at the top of each iteration because the way they're implemented
    // means it would be pointless to copy half of a cached shader
    if ((*pDataSize - bytes_written) < cachedShader.data_size) {
      return VK_INCOMPLETE;
    }

    // Now copy all the data from the cached shader into the buffer in a form
    // that can be serialized and used on different application runs. The layout
    // of the cached_shader in the buffer memory is identical to the
    // cached_shader struct definition except that the vectors are represented
    // by their size in a size_t followed by their data in a byte array.

    std::memcpy(cache_buffer + bytes_written, &cachedShader.data_size,
                sizeof(cachedShader.data_size));
    bytes_written += sizeof(cachedShader.data_size);

    std::memcpy(cache_buffer + bytes_written, &cachedShader.source_checksum,
                sizeof(cachedShader.source_checksum));
    bytes_written += sizeof(cachedShader.source_checksum);

    std::memcpy(cache_buffer + bytes_written,
                cachedShader.workgroup_size.data(),
                sizeof(cachedShader.workgroup_size));
    bytes_written += sizeof(cachedShader.workgroup_size);

    size_t binary_size = cachedShader.binary.size();
    std::memcpy(cache_buffer + bytes_written, &binary_size, sizeof(size_t));
    bytes_written += sizeof(size_t);

    std::memcpy(cache_buffer + bytes_written, cachedShader.binary.data(),
                binary_size);
    bytes_written += binary_size;

    size_t bindings_size = cachedShader.descriptor_bindings.empty()
                               ? 0
                               : cachedShader.descriptor_bindings.size() *
                                     sizeof(compiler::spirv::DescriptorBinding);
    std::memcpy(cache_buffer + bytes_written, &bindings_size, sizeof(size_t));
    bytes_written += sizeof(size_t);

    if (!cachedShader.descriptor_bindings.empty()) {
      std::memcpy(cache_buffer + bytes_written,
                  cachedShader.descriptor_bindings.data(), bindings_size);
      bytes_written += bindings_size;
    }

    (*shaders_written)++;
  }

  return bytes_written < *pDataSize ? VK_INCOMPLETE : VK_SUCCESS;
}

void DestroyPipelineCache(vk::device device, vk::pipeline_cache pipelineCache,
                          vk::allocator allocator) {
  if (pipelineCache == VK_NULL_HANDLE) {
    return;
  }

  (void)device;
  allocator.destroy(pipelineCache);
}
}  // namespace vk
