// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_BUFFER_H_INCLUDED
#define VK_BUFFER_H_INCLUDED

#include <mux/mux.hpp>
#include <vk/allocator.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @brief internal buffer type
typedef struct buffer_t final {
  /// @brief constructor
  ///
  /// @param mux_buffer mux buffer object to take ownership of
  /// @param usage Usage flags
  buffer_t(mux::unique_ptr<mux_buffer_t> &&mux_buffer,
           VkBufferUsageFlags usage);

  /// @brief destructor
  ~buffer_t();

  /// @brief Mux buffer object
  mux_buffer_t mux_buffer;

  /// @brief specifies what usages are allowed for this buffer
  VkBufferUsageFlags usage;
} * buffer;

/// @brief Internal implementation of vkCreateBuffer
///
/// @param device Device on which to create the buffer
/// @param pCreateInfo Create info
/// @param allocator Allocator
/// @param pBuffer Return created buffer
///
/// @return Vulkan result code
VkResult CreateBuffer(vk::device device, const VkBufferCreateInfo *pCreateInfo,
                      vk::allocator allocator, vk::buffer *pBuffer);

/// @brief Internal implementation of vkDestroyBuffer
///
/// @param device Device used to created the buffer
/// @param buffer Buffer to destroy
/// @param allocator Allocator
void DestroyBuffer(vk::device device, vk::buffer buffer,
                   vk::allocator allocator);

/// @brief Internal implementation of vkGetBufferMemoryRequirements
///
/// @param device the device on which the buffer was created
/// @param buffer the buffer to get the requirements of
/// @param pMemoryRequirements return memory requirements of the buffer
void GetBufferMemoryRequirements(vk::device device, vk::buffer buffer,
                                 VkMemoryRequirements *pMemoryRequirements);
}  // namespace vk

#endif  // VK_BUFFER_H_INCLUDED
