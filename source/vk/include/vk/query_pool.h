// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_QUERY_POOL_H_INCLUDED
#define VK_QUERY_POOL_H_INCLUDED

#include <vk/allocator.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t* device;

/// @brief Stub of internal query_pool type
typedef struct query_pool_t final {
} * query_pool;

/// @brief Stub of vkCreateQueryPool
VkResult CreateQueryPool(vk::device device,
                         const VkQueryPoolCreateInfo* pCreateInfo,
                         vk::allocator allocator, vk::query_pool* pQueryPool);

/// @brief Stub of vkDestroyQueryPool
void DestroyQueryPool(vk::device device, vk::query_pool queryPool,
                      vk::allocator allocator);

/// @brief Stub of vkGetQueryPoolResults
VkResult GetQueryPoolResults(vk::device device, vk::query_pool queryPool,
                             uint32_t firstQuery, uint32_t queryCount,
                             size_t dataSize, void* pData, VkDeviceSize stride,
                             VkQueryResultFlags flags);
}  // namespace vk

#endif
