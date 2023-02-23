// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_DESCRIPTOR_POOL_H_INCLUDED
#define VK_DESCRIPTOR_POOL_H_INCLUDED

#include <vk/allocator.h>
#include <vk/small_vector.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @copydoc ::vk::descriptor_binding_t
typedef struct descriptor_binding_t *descriptor_binding;

/// @copydoc ::vk::descriptor_set_t
typedef struct descriptor_set_t *descriptor_set;

/// @brief internal descriptor_pool type
typedef struct descriptor_pool_t final {
  /// @brief Constructor
  descriptor_pool_t(uint32_t max_sets,
                    VkDescriptorPoolCreateFlags create_flag_bits,
                    vk::allocator allocator);

  /// @brief Destructor
  ~descriptor_pool_t();

  /// @brief maximum number of descriptor sets that can be allocated from this
  /// pool
  uint32_t max_sets;

  /// @brief remaining number of descriptor sets that can be allocated from this
  /// pool
  uint32_t remaining_sets;

  /// @brief flag bits given at creation
  VkDescriptorPoolCreateFlags create_flag_bits;

  /// @brief allocator
  vk::allocator allocator;

  /// @brief list of all descriptor sets allocated from this pool
  vk::small_vector<vk::descriptor_set, 4> descriptor_sets;
} * descriptor_pool;

/// @brief internal implementation of vkCreateDescriptorPool
///
/// @param device Device on which to create the descriptor pool
/// @param pCreateInfo create info
/// @param allocator Allocator
/// @param pDescriptorPool return created descriptor pool
///
/// @return Return Vulkan result code
VkResult CreateDescriptorPool(vk::device device,
                              const VkDescriptorPoolCreateInfo *pCreateInfo,
                              vk::allocator allocator,
                              vk::descriptor_pool *pDescriptorPool);

/// @brief internal implementation of vkDestroyDescriptorPool
///
/// @param device device the descriptor pool was created on
/// @param descriptorPool the descriptor pool to be destroyed
/// @param allocator the allocator that was used to create the descriptor pool
void DestroyDescriptorPool(vk::device device,
                           vk::descriptor_pool descriptorPool,
                           vk::allocator allocator);

/// @brief internal implementation of vkResetDescriptorPool
///
/// @param device the device the descriptor pool was created on
/// @param descriptorPool the descriptor pool to reset
/// @param flags reserved for future use
///
/// @return Vulkan result code
VkResult ResetDescriptorPool(vk::device device,
                             vk::descriptor_pool descriptorPool,
                             VkDescriptorPoolResetFlags flags);
}  // namespace vk

#endif  // VK_DESCRIPTOR_POOL_H_INCLUDED
