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

/// @file
///
/// @brief Vulkan type traits.

#ifndef VK_TYPE_TRAITS_H_INCLUDED
#define VK_TYPE_TRAITS_H_INCLUDED

#include <cargo/type_traits.h>
#include <vulkan/vulkan.h>

namespace vk {
/// @brief Forward declare vk::instance.
typedef struct instance_t *instance;

/// @brief Forward declare vk::physical_device.
typedef struct physical_device_t *physical_device;

/// @brief Forward declare vk::device.
typedef struct device_t *device;

/// @brief Forward declare vk::queue.
typedef struct queue_t *queue;

/// @brief Forward declare vk::semaphore.
typedef struct semaphore_t *semaphore;

/// @brief Forward declare vk::command_buffer.
typedef struct command_buffer_t *command_buffer;

/// @brief Forward declare vk::fence.
typedef struct fence_t *fence;

/// @brief Forward declare vk::device_memory.
typedef struct device_memory_t *device_memory;

/// @brief Forward declare vk::buffer.
typedef struct buffer_t *buffer;

/// @brief Forward declare vk::image.
typedef struct image_t *image;

/// @brief Forward declare vk::event.
typedef struct event_t *event;

/// @brief Forward declare vk::query_pool.
typedef struct query_pool_t *query_pool;

/// @brief Forward declare vk::buffer_view.
typedef struct buffer_view_t *buffer_view;

/// @brief Forward declare vk::image_view.
typedef struct image_view_t *image_view;

/// @brief Forward declare vk::shader_module.
typedef struct shader_module_t *shader_module;

/// @brief Forward declare vk::pipeline_cache.
typedef struct pipeline_cache_t *pipeline_cache;

/// @brief Forward declare vk::pipeline_layout.
typedef struct pipeline_layout_t *pipeline_layout;

/// @brief Forward declare vk::render_pass.
typedef struct render_pass_t *render_pass;

/// @brief Forward declare vk::pipeline.
typedef struct pipeline_t *pipeline;

/// @brief Forward declare vk::descriptor_set_layout.
typedef struct descriptor_set_layout_t *descriptor_set_layout;

/// @brief Forward declare vk::sampler.
typedef struct sampler_t *sampler;

/// @brief Forward declare vk::descriptor_pool.
typedef struct descriptor_pool_t *descriptor_pool;

/// @brief Forward declare vk::descriptor_set.
typedef struct descriptor_set_t *descriptor_set;

/// @brief Forward declare vk::framebuffer.
typedef struct framebuffer_t *framebuffer;

/// @brief Forward declare vk::command_pool.
typedef struct command_pool_t *command_pool;

/// @brief Type trait to determine if a vulkan handle can be converted.
///
/// The catch all case defaults to `std::false_type`, valid specialzations
/// inherit from `std::true_type`.
template <class T, class U>
struct is_convertible_to : std::false_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::instance`.
template <>
struct is_convertible_to<vk::instance, VkInstance> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::physical_device`.
template <>
struct is_convertible_to<vk::physical_device, VkPhysicalDevice>
    : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::device`.
template <>
struct is_convertible_to<vk::device, VkDevice> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::queue`.
template <>
struct is_convertible_to<vk::queue, VkQueue> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::semaphore`.
template <>
struct is_convertible_to<vk::semaphore, VkSemaphore> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::command_buffer`.
template <>
struct is_convertible_to<vk::command_buffer, VkCommandBuffer> : std::true_type {
};

/// @brief Specialize `vk::is_convertible_to` for `vk::fence`.
template <>
struct is_convertible_to<vk::fence, VkFence> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::device_memory`.
template <>
struct is_convertible_to<vk::device_memory, VkDeviceMemory> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::buffer`.
template <>
struct is_convertible_to<vk::buffer, VkBuffer> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::image`.
template <>
struct is_convertible_to<vk::image, VkImage> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::event`.
template <>
struct is_convertible_to<vk::event, VkEvent> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::query_pool`.
template <>
struct is_convertible_to<vk::query_pool, VkQueryPool> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::buffer_view`.
template <>
struct is_convertible_to<vk::buffer_view, VkBufferView> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::image_view`.
template <>
struct is_convertible_to<vk::image_view, VkImageView> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::shader_module`.
template <>
struct is_convertible_to<vk::shader_module, VkShaderModule> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::pipeline_cache`.
template <>
struct is_convertible_to<vk::pipeline_cache, VkPipelineCache> : std::true_type {
};

/// @brief Specialize `vk::is_convertible_to` for `vk::pipeline_layout`.
template <>
struct is_convertible_to<vk::pipeline_layout, VkPipelineLayout>
    : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::render_pass`.
template <>
struct is_convertible_to<vk::render_pass, VkRenderPass> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::pipeline`.
template <>
struct is_convertible_to<vk::pipeline, VkPipeline> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::descriptor_set_layout`.
template <>
struct is_convertible_to<vk::descriptor_set_layout, VkDescriptorSetLayout>
    : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::sampler`.
template <>
struct is_convertible_to<vk::sampler, VkSampler> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::descriptor_pool`.
template <>
struct is_convertible_to<vk::descriptor_pool, VkDescriptorPool>
    : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::descriptor_set`.
template <>
struct is_convertible_to<vk::descriptor_set, VkDescriptorSet> : std::true_type {
};

/// @brief Specialize `vk::is_convertible_to` for `vk::framebuffer`.
template <>
struct is_convertible_to<vk::framebuffer, VkFramebuffer> : std::true_type {};

/// @brief Specialize `vk::is_convertible_to` for `vk::command_pool`.
template <>
struct is_convertible_to<vk::command_pool, VkCommandPool> : std::true_type {};

/// @brief Safely cast from `Vk<Type>` to `vk::<type>`.
///
/// @tparam T Detination type of the cast.
/// @tparam U Source type of the cast.
/// @param u Instance of type `U` to cast to type `T`.
///
/// @return Returns `u` cast to type `T`.
template <class T, class U>
std::enable_if_t<is_convertible_to<T, U>::value, T> cast(U u) {
  return reinterpret_cast<T>(u);
}

/// @brief Safely cast from `Vk<Type>*` to `vk::<type>*`.
///
/// @tparam T Detination type of the cast.
/// @tparam U Source type of the cast.
/// @param u Instance of type `U` to cast to type `T`.
///
/// @return Returns `u` cast to type `T`.
template <class T, class U>
std::enable_if_t<is_convertible_to<std::remove_pointer_t<T>, U>::value, T> cast(
    U *u) {
  return reinterpret_cast<T>(u);
}

/// @brief Safely cast from `const Vk<Type>*` to `const vk::<type>*`.
///
/// @tparam T Detination type of the cast.
/// @tparam U Source type of the cast.
/// @param u Instance of type `U` to cast to type `T`.
///
/// @return Returns `u` cast to type `T`.
template <class T, class U>
std::enable_if_t<
    std::is_pointer_v<T> &&
        is_convertible_to<std::remove_const_t<std::remove_pointer_t<T>>,
                          std::remove_const_t<U>>::value,
    T>
cast(const U *u) {
  return reinterpret_cast<T>(u);
}
}  // namespace vk

#endif  // VK_TYPE_TRAITS_H_INCLUDED
