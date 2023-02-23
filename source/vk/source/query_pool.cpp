// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vk/device.h>
#include <vk/query_pool.h>

namespace vk {
VkResult CreateQueryPool(vk::device device,
                         const VkQueryPoolCreateInfo* pCreateInfo,
                         vk::allocator allocator, vk::query_pool* pQueryPool) {
  (void)device;
  (void)pCreateInfo;
  (void)allocator;
  (void)pQueryPool;

  return VK_ERROR_FEATURE_NOT_PRESENT;
}

void DestroyQueryPool(vk::device device, vk::query_pool queryPool,
                      vk::allocator allocator) {
  (void)device;
  (void)queryPool;
  (void)allocator;
}

VkResult GetQueryPoolResults(vk::device device, vk::query_pool queryPool,
                             uint32_t firstQuery, uint32_t queryCount,
                             size_t dataSize, void* pData, VkDeviceSize stride,
                             VkQueryResultFlags flags) {
  (void)device;
  (void)queryPool;
  (void)firstQuery;
  (void)queryCount;
  (void)dataSize;
  (void)pData;
  (void)stride;
  (void)flags;

  return VK_ERROR_FEATURE_NOT_PRESENT;
}
}  // namespace vk
