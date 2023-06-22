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

#ifndef VK_PIPELINE_CACHE_H_INCLUDED
#define VK_PIPELINE_CACHE_H_INCLUDED

#include <mux/mux.h>
#include <vk/allocator.h>
#include <vk/icd.h>
#include <vk/small_vector.h>
#include <vulkan/vulkan.h>

#include <array>
#include <cstring>
#include <mutex>

namespace vk {

/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @brief Struct representing pipeline cache entry
struct cached_shader {
  /// @brief Default constructor.
  cached_shader(const VkAllocationCallbacks *pAllocator,
                VkSystemAllocationScope allocationScope)
      : source_checksum(),
        workgroup_size(),
        binary(cargo_allocator<uint8_t>(pAllocator, allocationScope)),
        descriptor_bindings(cargo_allocator<compiler::spirv::DescriptorBinding>(
            pAllocator, allocationScope)) {}

  /// @brief Move constructor.
  ///
  /// @param other Other cached shader to move from.
  cached_shader(cached_shader &&other)
      : data_size(other.data_size),
        source_checksum(other.source_checksum),
        workgroup_size(std::move(other.workgroup_size)),
        binary(std::move(other.binary)),
        descriptor_bindings(std::move(other.descriptor_bindings)) {}

  /// @brief Move assignment operator.
  ///
  /// @param other Other cached shader to moved from.
  ///
  /// @return Returns a reference to this cached shader.
  cached_shader &operator=(cached_shader &&other);

  /// @brief Create a clone of this cached shader.
  ///
  /// @return Returns the cloned cached shader, or an error.
  /// @retval `cargo::bad_alloc` if an allocation failed.
  cargo::error_or<cached_shader> clone() const;

  /// @brief Check for equality with another cached shader.
  ///
  /// @param other Cache shader to compare.
  ///
  /// @return Returns true if checksum's match, false otherwise.
  bool operator==(const cached_shader &other) const;

  /// @brief Check for equality with a checksum.
  ///
  /// @param checksum Checksum value to compare.
  ///
  /// @return Returns true if checksum's match, false, otherwise.
  bool operator==(const uint32_t &checksum) const;

  /// @brief Total size in bytes of all the data encoded in this cache entry
  size_t data_size;
  /// @brief Source SPIR-V binary's checksum
  uint32_t source_checksum;
  /// @brief Local workgroup size defined by the shader, cached at translation
  std::array<uint32_t, 3> workgroup_size;
  /// @brief Cached llvm bitcode.
  vk::small_vector<uint8_t, 128> binary;
  /// @brief Descriptor slots used by the cached shader
  vk::small_vector<compiler::spirv::DescriptorBinding, 2> descriptor_bindings;
};

/// @brief internal pipeline cache type
typedef struct pipeline_cache_t final : icd_t<pipeline_cache_t> {
  /// @brief Constructor
  ///
  /// @param allocator Allocator used to initialize the list of cache entries
  pipeline_cache_t(vk::allocator allocator);

  /// @brief Destructor
  ~pipeline_cache_t() {}

  /// @brief Data cached from pipeline creation
  vk::small_vector<cached_shader, 2> cache_entries;

  /// @brief Mutex used for locking during access to `cache_entries`
  std::mutex mutex;
} * pipeline_cache;

/// @brief Internal implementation of vkCreatePipelineCache
///
/// @param device Device that will own the pipeline cache
/// @param pCreateInfo Pipeline cache create info struct
/// @param allocator Allocation callbacks to use in creating the pipeline cache
/// @param pPipelineCache Pointer in which to return the created pipeline cache
///
/// @return Vulkan result code
VkResult CreatePipelineCache(vk::device device,
                             const VkPipelineCacheCreateInfo *pCreateInfo,
                             vk::allocator allocator,
                             vk::pipeline_cache *pPipelineCache);

/// @brief Internal implementation of vkMergePipelineCaches
///
/// @param device Device which owns the pipeline caches involved
/// @param dstCache Pipeline cache to merge into
/// @param srcCacheCount Number of elements in `pSrcCaches`
/// @param pSrcCaches Array of `srcCacheCount` pipeline caches to merge
///
/// @retun Vulkan result code
VkResult MergePipelineCaches(vk::device device, vk::pipeline_cache dstCache,
                             uint32_t srcCacheCount,
                             const VkPipelineCache *pSrcCaches);

/// @brief Internal implementation of vkGetPipelineCache
///
/// @param device Device which owns the pipeline cache
/// @param pipelineCache Pipeline cache object to get data from
/// @param pDataSize Size in bytes of data to retrieve
/// @param pData Pointer to a buffer in which to return the data
///
/// @return Vulkan result code
VkResult GetPipelineCacheData(vk::device device,
                              vk::pipeline_cache pipelineCache,
                              size_t *pDataSize, void *pData);

/// @brief Internal implementation of vkDestroyPipelineCache
///
/// @param device Device which owns the pipeline cache
/// @param pipelineCache Pipeline cache object to destroy
/// @param allocator Allocator that was used to create the pipeline cache
void DestroyPipelineCache(vk::device device, vk::pipeline_cache pipelineCache,
                          vk::allocator allocator);
}  // namespace vk

#endif  // VK_PIPELINE_CACHE_H_INCLUDED
