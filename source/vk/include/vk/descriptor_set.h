// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_DESCRIPTOR_SET_H_INCLUDED
#define VK_DESCRIPTOR_SET_H_INCLUDED

#include <mux/mux.h>
#include <vk/allocator.h>
#include <vk/error.h>
#include <vk/small_vector.h>

#include <algorithm>

namespace vk {
/// @copydoc ::vk::buffer_t
typedef struct buffer_t *buffer;

/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @copydoc ::vk::descriptor_pool_t
typedef struct descriptor_pool_t *descriptor_pool;

/// @copydoc ::vk::descriptor_set_layout_t
typedef struct descriptor_set_layout_t *descriptor_set_layout;

/// @brief struct that represent a binding in a descriptor set
typedef struct descriptor_binding_t final {
  /// @brief constructor
  descriptor_binding_t(uint32_t descriptor_count,
                       mux_descriptor_info_t *descriptors)
      : descriptor_count(descriptor_count),
        descriptors(descriptors),
        dynamic(false) {}

  /// @brief destructor
  ~descriptor_binding_t() {}

  /// @brief length of @p descriptors
  uint32_t descriptor_count;

  /// @brief The descriptors in this binding
  mux_descriptor_info_t *descriptors;

  /// @brief whether this binding will be used for dynamic resources
  bool dynamic;
} * descriptor_binding;

/// @brief internal descriptor set type
typedef struct descriptor_set_t final {
  /// @brief Constructor
  ///
  /// @param allocator `vk::allocator` used to initialize
  /// `descriptor_bindings`
  descriptor_set_t(vk::allocator allocator);

  /// @brief Destructor
  ~descriptor_set_t();

  /// @brief The bindings in this descriptor set
  vk::small_vector<vk::descriptor_binding, 4> descriptor_bindings;
} * descriptor_set;

/// @brief Internal implementation of vkAllocateDescriptorSets
///
/// @param device device on which the descriptor set(s) will be allocated
/// @param pAllocateInfo allocate info
/// @param pDescriptorSets return handles to allocated descriptor sets
///
/// @return return Vulkan result code
VkResult AllocateDescriptorSets(
    vk::device device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
    VkDescriptorSet *pDescriptorSets);

/// @brief internal implementation of vkFreeDescriptorSets
///
/// @param device Device on which the descriptor set(s) were allocated
/// @param descriptorPool Descriptor pool from which the descriptor set(s) were
/// allocated
/// @param descriptorSetCount the length of pDescriptorSets
/// @param pDescriptorSets descriptor set(s) to destroy
///
/// @return return Vulkan result code
VkResult FreeDescriptorSets(vk::device device,
                            vk::descriptor_pool descriptorPool,
                            uint32_t descriptorSetCount,
                            const vk::descriptor_set *pDescriptorSets);

/// @brief internal implementation of vkUpdateDescriptorSets
///
/// @param device Device the descriptor sets were created on
/// @param descriptorWriteCount length of pDescriptorWrites
/// @param pDescriptorWrites Array of structures describing the descriptor sets
/// to write to
/// @param descriptorCopyCount length of pDescriptorCopies
/// @param pDescriptorCopies Array of structures describing the descriptor sets
/// to copy between
void UpdateDescriptorSets(vk::device device, uint32_t descriptorWriteCount,
                          const VkWriteDescriptorSet *pDescriptorWrites,
                          uint32_t descriptorCopyCount,
                          const VkCopyDescriptorSet *pDescriptorCopies);
}  // namespace vk

#endif  // VK_DESCRIPTOR_SET_H_INCLUDED
