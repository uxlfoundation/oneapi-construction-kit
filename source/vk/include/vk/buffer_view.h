// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_BUFFER_VIEW_INCLUDED
#define VK_BUFFER_VIEW_INCLUDED

#include <mux/mux.h>
#include <vk/allocator.h>

namespace vk {
/// @copydoc ::vk::device_t
typedef struct device_t *device;

/// @copydoc ::vk::buffer_t
typedef struct buffer_t *buffer;

/// @brief internal buffer_view type
typedef struct buffer_view_t final {
  /// @brief Constructor
  buffer_view_t(mux_buffer_t buffer, VkFormat format, size_t offset,
                size_t range);

  /// @brief Destructor
  ~buffer_view_t();

  /// @brief buffer the view is created on
  mux_buffer_t buffer;

  /// @brief format of the data elements in the buffer
  VkFormat format;

  /// @brief an offset in bytes from the base address of the buffer
  size_t offset;

  /// @brief range in bytes, or VK_WHOLE_SIZE for the whole buffer
  size_t range;
} * buffer_view;

/// @brief Internal implementation of vkCreateBufferView
///
/// @param device Device the buffer was created on.
/// @param pCreateInfo Create info.
/// @param allocator Allocator.
/// @param pView Return created buffer view.
VkResult CreateBufferView(vk::device device,
                          const VkBufferViewCreateInfo *pCreateInfo,
                          vk::allocator allocator, vk::buffer_view *pView);

/// @brief Internal implementation of vkDestroyBufferView
///
/// @param device Device the buffer view was created on
/// @param bufferView The buffer view to destroy
/// @param allocator Allocator
void DestroyBufferView(vk::device device, vk::buffer_view bufferView,
                       vk::allocator allocator);

}  // namespace vk

#endif  // VK_BUFFER_H_INCLUDED
