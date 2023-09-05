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
/// @brief The Mux API.
///
/// @note This file was automatically generated from an XML API specification.

#ifndef MUX_MUX_H_INCLUDED
#define MUX_MUX_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/// @addtogroup mux
/// @{

/// @brief Mux major version number.
#define MUX_MAJOR_VERSION 0
/// @brief Mux minor version number.
#define MUX_MINOR_VERSION 79
/// @brief Mux patch version number.
#define MUX_PATCH_VERSION 0
/// @brief Mux combined version number.
#define MUX_VERSION                                                            \
  (((uint32_t)MUX_MAJOR_VERSION << 22) | ((uint32_t)MUX_MINOR_VERSION << 12) | \
   ((uint32_t)MUX_PATCH_VERSION))

/// @brief Default value for initializing Mux object `id` member variable.
#define MUX_NULL_ID 0

/// @brief A result code type.
///
/// The return code from our Mux functions.
///
/// @see mux_result_e
typedef int32_t mux_result_t;

/// @brief Declare Mux's combined identifier type.
typedef uint32_t mux_id_t;

/// @brief Declare Mux's object identifier type.
typedef uint32_t mux_object_id_t;

/// @brief Declare Mux's target identifier type.
typedef uint32_t mux_target_id_t;

/// @brief Declare Mux's command identifier type.
typedef uint32_t mux_command_id_t;

/// @brief Forward declare Mux's device information container.
typedef struct mux_device_info_s *mux_device_info_t;

/// @brief Forward declare Mux's device container.
typedef struct mux_device_s *mux_device_t;

/// @brief Forward declare Mux's memory container.
typedef struct mux_memory_s *mux_memory_t;

/// @brief Forward declare Mux's buffer container.
typedef struct mux_buffer_s *mux_buffer_t;

/// @brief Forward declare Mux's three dimensional offset.
typedef struct mux_offset_3d_s mux_offset_3d_t;

/// @brief Forward declare Mux's three dimensional extent.
typedef struct mux_extent_3d_s mux_extent_3d_t;

/// @brief Forward declare Mux's two dimensional extent.
typedef struct mux_extent_2d_s mux_extent_2d_t;

/// @brief Forward declare Mux's image container.
typedef struct mux_image_s *mux_image_t;

/// @brief Forward declare Mux's sampler container.
typedef struct mux_sampler_s mux_sampler_t;

/// @brief Forward declare Mux's queue container.
typedef struct mux_queue_s *mux_queue_t;

/// @brief Forward declare Mux's command buffer container.
typedef struct mux_command_buffer_s *mux_command_buffer_t;

/// @brief Forward declare Mux's semaphore container.
typedef struct mux_semaphore_s *mux_semaphore_t;

/// @brief Forward declare Mux's fence container.
typedef struct mux_fence_s *mux_fence_t;

/// @brief Forward declare Mux's sync-point container.
typedef struct mux_sync_point_s *mux_sync_point_t;

/// @brief Forward declare Mux's executable container.
typedef struct mux_executable_s *mux_executable_t;

/// @brief Forward declare Mux's kernel container.
typedef struct mux_kernel_s *mux_kernel_t;

/// @brief Forward declare Mux's query pool container.
typedef struct mux_query_pool_s *mux_query_pool_t;

/// @brief Forward declare Mux's query counter container.
typedef struct mux_query_counter_s mux_query_counter_t;

/// @brief Forward declare Mux's query counter description container.
typedef struct mux_query_counter_description_s mux_query_counter_description_t;

/// @brief Forward declare Mux's query counter configuration container.
typedef struct mux_query_counter_config_s mux_query_counter_config_t;

/// @brief Forward declare Mux's duration query result container.
typedef struct mux_query_duration_result_s *mux_query_duration_result_t;

/// @brief Forward declare Mux's counter query result container.
typedef struct mux_query_counter_result_s *mux_query_counter_result_t;

/// @brief Forward declare Mux's allocator container.
typedef struct mux_allocator_info_s mux_allocator_info_t;

/// @brief Forward declare Mux's callback container.
typedef struct mux_callback_info_s *mux_callback_info_t;

/// @brief Forward declare Mux's descriptor container.
typedef struct mux_descriptor_info_s mux_descriptor_info_t;

/// @brief Forward declare Mux's buffer region container.
typedef struct mux_buffer_region_info_s mux_buffer_region_info_t;

/// @brief Forward declare Mux's kernel execution options container.
typedef struct mux_ndrange_options_s mux_ndrange_options_t;

/// @brief All Mux result types.
///
/// Used as the return codes from our Mux functions, to denote whether the
/// function was successful.
enum mux_result_e {
  /// @brief No error occurred.
  mux_success = 0,
  /// @brief An unknown error occurred.
  mux_error_failure = 1,
  /// @brief An out parameter (EG. a pointer argument) was null when it should
  /// not be.
  mux_error_null_out_parameter = 2,
  /// @brief An invalid parameter was provided.
  mux_error_invalid_value = 3,
  /// @brief An allocation failed.
  mux_error_out_of_memory = 4,
  /// @brief An allocator callback provided to Mux was null.
  mux_error_null_allocator_callback = 5,
  /// @brief A device entry hook was malformed.
  mux_error_device_entry_hook_failed = 6,
  /// @brief A file was an invalid binary.
  mux_error_invalid_binary = 7,
  /// @brief Feature is unsupported by Mux API.
  mux_error_feature_unsupported = 8,
  /// @brief Kernel was not found.
  mux_error_missing_kernel = 9,
  /// @brief An internal error has occurred, this should never happen. Use this
  /// in place of assertions.
  mux_error_internal = 10,
  /// @brief A fence waited upon failed.
  mux_error_fence_failure = 11,
  /// @brief A fence waited upon was not yet complete.
  mux_fence_not_ready = 12
};

/// @brief All Mux object type tags.
///
/// Type IDs are combined with the generated `mux_target_id_e`, using a bitwise
/// and, to create the value for each Mux object's `id` member, all
/// `::mux_object_id_e` values are in the upper 16 bits of the 32 bit
/// `::mux_id_t`.
enum mux_object_id_e {
  /// @brief ID for `::mux_buffer_t` objects.
  mux_object_id_buffer = (1 << 16),
  /// @brief ID for `::mux_command_buffer_t` objects.
  mux_object_id_command_buffer = (2 << 16),
  /// @brief ID for `::mux_device_t` objects.
  mux_object_id_device = (3 << 16),
  /// @brief ID for `::mux_executable_t` objects.
  mux_object_id_executable = (4 << 16),
  /// @brief ID for `::mux_image_t` objects.
  mux_object_id_image = (5 << 16),
  /// @brief ID for `::mux_kernel_t` objects.
  mux_object_id_kernel = (6 << 16),
  /// @brief ID for `::mux_memory_t` objects.
  mux_object_id_memory = (7 << 16),
  /// @brief ID for `::mux_queue_t` objects.
  mux_object_id_queue = (8 << 16),
  /// @brief ID for `::mux_sampler_t` objects.
  mux_object_id_sampler = (9 << 16),
  /// @brief ID for `::mux_semaphore_t` objects.
  mux_object_id_semaphore = (10 << 16),
  /// @brief ID for `::mux_query_pool_t` objects.
  mux_object_id_query_pool = (11 << 16),
  /// @brief ID for `::mux_sync_point_t` objects.
  mux_object_id_sync_point = (12 << 16),
  /// @brief ID for `::mux_fence_t` objects.
  mux_object_id_fence = (13 << 16),
  /// @brief Mask for ID values which are always help in the upper 16 bits of
  /// the 32 bit `::mux_id_t`.
  mux_object_id_mask = 0xffff0000
};

/// @brief All possible device types.
///
/// Used to identify which type a Mux device is, can be used as a bitfield to
/// select devices by type with `::muxGetDeviceInfos`.
///
/// @see mux_device_info_s::device_type
enum mux_device_type_e {
  /// @brief Device is a CPU.
  mux_device_type_cpu = (0x1 << 0),
  /// @brief Device is an integrated GPU.
  mux_device_type_gpu_integrated = (0x1 << 1),
  /// @brief Device is a discrete GPU.
  mux_device_type_gpu_discrete = (0x1 << 2),
  /// @brief Device is a virtualized GPU.
  mux_device_type_gpu_virtual = (0x1 << 3),
  /// @brief Device is an accelerator chip.
  mux_device_type_accelerator = (0x1 << 4),
  /// @brief Device is a mysterious custom type.
  mux_device_type_custom = (0x1 << 5),
  /// @brief Mask to match all device types.
  mux_device_type_all = 0xFFFFFFFF
};

/// @brief All possible queue types that can be supported.
///
/// Used to identify which queue type is required for operations.
///
/// @note There exists code that assumes that this enum starts at zero, and that
/// the numbers increment contiguously until mux_queue_type_total.
typedef enum mux_queue_type_e {
  /// @brief A queue type that supports all compute operations.
  mux_queue_type_compute = 0,
  /// @brief The total number of queue types used.
  mux_queue_type_total = 1
} mux_queue_type_e;

/// @brief All possible allocation types that can be supported.
///
/// Used when allocating memory through the Mux device to specify what
/// allocation type is being requested.
typedef enum mux_allocation_type_e {
  /// @brief Allocate pinned memory visible on both host and device. Used in
  /// combination with mux_memory_property_host_coherent or
  /// mux_memory_property_host_cached.
  mux_allocation_type_alloc_host = 0,
  /// @brief Allocate device memory. Can be used with
  /// mux_memory_property_device_local or mux_memory_property_host_visible.
  mux_allocation_type_alloc_device = 1
} mux_allocation_type_e;

/// @brief Bitfield of all possible allocation capabilities.
///
/// Each Mux device info struct has a member which denotes the allocation
/// capabilities of that device, as a bitfield of the following enum.
///
/// @see mux_device_info_s::allocation_capabilities
enum mux_allocation_capabilities_e {
  /// @brief Can a Mux device allocate cache coherent host memory, or use
  /// pre-allocated host memory without synchronization.
  mux_allocation_capabilities_coherent_host = 0x1 << 0,
  /// @brief Can a Mux device use host memory with explicit synchronization.
  mux_allocation_capabilities_cached_host = 0x1 << 1,
  /// @brief Can a Mux device allocate device-only memory.
  mux_allocation_capabilities_alloc_device = 0x1 << 2
};

/// @brief Each of the memory properties which may be requested for an
/// allocation.
typedef enum mux_memory_property_e {
  /// @brief Memory is visible only on the device and can not be mapped. Must
  /// not be used in combination with mux_memory_property_host_visible,
  /// mux_memory_property_host_coherent, or mux_memory_property_host_cached.
  mux_memory_property_device_local = (0x1 << 1),
  /// @brief Memory is visible on host and device, can be mapped using
  /// muxMapMemory. Can be used with mux_memory_property_host_coherent or
  /// mux_memory_property_host_cached.
  mux_memory_property_host_visible = (0x1 << 2),
  /// @brief Memory caches are coherent between host and device. No explicit
  /// synchronization is required. Can be used with
  /// mux_memory_property_host_visible, is mutually exclusive with
  /// mux_memory_property_host_cached.
  mux_memory_property_host_coherent = (0x1 << 3),
  /// @brief Memory caches are not coherent between host and device. Explicit
  /// synchronization is required, using muxMapMemory and muxUnmapMemory, before
  /// updates are visible on host or device. Can be used with
  /// mux_memory_property_host_visible, is mutually exclusive with
  /// mux_memory_property_host_coherent.
  mux_memory_property_host_cached = (0x1 << 4)
} mux_memory_property_e;

/// @brief All possible image types that can be supported.
///
/// Used when creating a mux_image_t object to specify the dimensionality of the
/// image.
typedef enum mux_image_type_e {
  /// @brief A 1 dimensional image type.
  mux_image_type_1d,
  /// @brief A 2 dimensional image type.
  mux_image_type_2d,
  /// @brief A 3 dimensional image type.
  mux_image_type_3d,
  /// @brief Force enum size to 32 bits
  mux_force_size_image_type = 0x7fffffff
} mux_image_type_e;

/// @brief All possible image tiling modes an image can be in.
typedef enum mux_image_tiling_e {
  /// @brief Linear tiling mode lays out pixel data in linear order, this order
  /// should be used when transferring data to/from the host.
  mux_image_tiling_linear,
  /// @brief Optimal tiling mode lays out pixel data in cache friendly order,
  /// this mode should be used for best device performance.
  mux_image_tiling_optimal
} mux_image_tiling_e;

/// @brief All possible image formats that can be supported.
///
/// Used when creating a ::mux_image_t object to specify the pixel data format
/// of the image. The enum values are constructed from the hexadecimal constants
/// specified by OpenCL, the lower 16 bits contains the cl_channel_order value
/// and the upper 16 bits contains the cl_channel_type value.
typedef enum mux_image_format_e {
  /// @brief R channel format with 8 signed normalized bits per channel.
  mux_image_format_R8_snorm = (0x10B0 | (0x10D0 << 16)),
  /// @brief R channel format with 16 signed normalized bits per channel.
  mux_image_format_R16_snorm = (0x10B0 | (0x10D1 << 16)),
  /// @brief R channel format with 8 unsigned normalized bits per channel.
  mux_image_format_R8_unorm = (0x10B0 | (0x10D2 << 16)),
  /// @brief R channel format with 16 unsigned normalized bits per channel.
  mux_image_format_R16_unorm = (0x10B0 | (0x10D3 << 16)),
  /// @brief R channel format with 8 signed integer bits per channel.
  mux_image_format_R8_sint = (0x10B0 | (0x10D7 << 16)),
  /// @brief R channel format with 16 signed integer bits per channel.
  mux_image_format_R16_sint = (0x10B0 | (0x10D8 << 16)),
  /// @brief R channel format with 32 signed integer bits per channel.
  mux_image_format_R32_sint = (0x10B0 | (0x10D9 << 16)),
  /// @brief R channel format with 8 unsigned integer bits per channel.
  mux_image_format_R8_uint = (0x10B0 | (0x10DA << 16)),
  /// @brief R channel format with 16 unsigned integer bits per channel.
  mux_image_format_R16_uint = (0x10B0 | (0x10DB << 16)),
  /// @brief R channel format with 32 unsigned integer bits per channel.
  mux_image_format_R32_uint = (0x10B0 | (0x10DC << 16)),
  /// @brief R channel format with 16 signed floating point bits per channel.
  mux_image_format_R16_sfloat = (0x10B0 | (0x10DD << 16)),
  /// @brief R channel format with 32 signed floating point bits per channel.
  mux_image_format_R32_sfloat = (0x10B0 | (0x10DE << 16)),
  /// @brief A channel format with 8 signed normalized bits per channel.
  mux_image_format_A8_snorm = (0x10B1 | (0x10D0 << 16)),
  /// @brief A channel format with 16 signed normalized bits per channel.
  mux_image_format_A16_snorm = (0x10B1 | (0x10D1 << 16)),
  /// @brief A channel format with 8 unsigned normalized bits per channel.
  mux_image_format_A8_unorm = (0x10B1 | (0x10D2 << 16)),
  /// @brief A channel format with 16 unsigned normalized bits per channel.
  mux_image_format_A16_unorm = (0x10B1 | (0x10D3 << 16)),
  /// @brief A channel format with 8 signed integer bits per channel.
  mux_image_format_A8_sint = (0x10B1 | (0x10D7 << 16)),
  /// @brief A channel format with 16 signed integer bits per channel.
  mux_image_format_A16_sint = (0x10B1 | (0x10D8 << 16)),
  /// @brief A channel format with 32 signed integer bits per channel.
  mux_image_format_A32_sint = (0x10B1 | (0x10D9 << 16)),
  /// @brief A channel format with 8 unsigned integer bits per channel.
  mux_image_format_A8_uint = (0x10B1 | (0x10DA << 16)),
  /// @brief A channel format with 16 unsigned integer bits per channel.
  mux_image_format_A16_uint = (0x10B1 | (0x10DB << 16)),
  /// @brief A channel format with 32 unsigned integer bits per channel.
  mux_image_format_A32_uint = (0x10B1 | (0x10DC << 16)),
  /// @brief A channel format with 16 signed floating point bits per channel.
  mux_image_format_A16_sfloat = (0x10B1 | (0x10DD << 16)),
  /// @brief A channel format with 32 signed floating point bits per channel.
  mux_image_format_A32_sfloat = (0x10B1 | (0x10DE << 16)),
  /// @brief RG channel format with 8 signed normalized bits per channel.
  mux_image_format_R8G8_snorm = (0x10B2 | (0x10D0 << 16)),
  /// @brief RG channel format with 16 signed normalized bits per channel.
  mux_image_format_R16G16_snorm = (0x10B2 | (0x10D1 << 16)),
  /// @brief RG channel format with 8 unsigned normalized bits per channel.
  mux_image_format_R8G8_unorm = (0x10B2 | (0x10D2 << 16)),
  /// @brief RG channel format with 16 unsigned normalized bits per channel.
  mux_image_format_R16G16_unorm = (0x10B2 | (0x10D3 << 16)),
  /// @brief RG channel format with 8 signed integer bits per channel.
  mux_image_format_R8G8_sint = (0x10B2 | (0x10D7 << 16)),
  /// @brief RG channel format with 16 signed integer bits per channel.
  mux_image_format_R16G16_sint = (0x10B2 | (0x10D8 << 16)),
  /// @brief RG channel format with 32 signed integer bits per channel.
  mux_image_format_R32G32_sint = (0x10B2 | (0x10D9 << 16)),
  /// @brief RG channel format with 8 unsigned integer bits per channel.
  mux_image_format_R8G8_uint = (0x10B2 | (0x10DA << 16)),
  /// @brief RG channel format with 16 unsigned integer bits per channel.
  mux_image_format_R16G16_uint = (0x10B2 | (0x10DB << 16)),
  /// @brief RG channel format with 32 unsigned integer bits per channel.
  mux_image_format_R32G32_uint = (0x10B2 | (0x10DC << 16)),
  /// @brief RG channel format with 16 signed floating point bits per channel.
  mux_image_format_R16G16_sfloat = (0x10B2 | (0x10DD << 16)),
  /// @brief RG channel format with 32 signed floating point bits per channel.
  mux_image_format_R32G32_sfloat = (0x10B2 | (0x10DE << 16)),
  /// @brief RA channel format with 8 signed normalized bits per channel.
  mux_image_format_R8A8_snorm = (0x10B3 | (0x10D0 << 16)),
  /// @brief RA channel format with 16 signed normalized bits per channel.
  mux_image_format_R16A16_snorm = (0x10B3 | (0x10D1 << 16)),
  /// @brief RA channel format with 8 unsigned normalized bits per channel.
  mux_image_format_R8A8_unorm = (0x10B3 | (0x10D2 << 16)),
  /// @brief RA channel format with 16 unsigned normalized bits per channel.
  mux_image_format_R16A16_unorm = (0x10B3 | (0x10D3 << 16)),
  /// @brief RA channel format with 8 signed integer bits per channel.
  mux_image_format_R8A8_sint = (0x10B3 | (0x10D7 << 16)),
  /// @brief RA channel format with 16 signed integer bits per channel.
  mux_image_format_R16A16_sint = (0x10B3 | (0x10D8 << 16)),
  /// @brief RA channel format with 32 signed integer bits per channel.
  mux_image_format_R32A32_sint = (0x10B3 | (0x10D9 << 16)),
  /// @brief RA channel format with 8 unsigned integer bits per channel.
  mux_image_format_R8A8_uint = (0x10B3 | (0x10DA << 16)),
  /// @brief RA channel format with 16 unsigned integer bits per channel.
  mux_image_format_R16A16_uint = (0x10B3 | (0x10DB << 16)),
  /// @brief RA channel format with 32 unsigned integer bits per channel.
  mux_image_format_R32A32_uint = (0x10B3 | (0x10DC << 16)),
  /// @brief RA channel format with 16 signed floating point bits per channel.
  mux_image_format_R16A16_sfloat = (0x10B3 | (0x10DD << 16)),
  /// @brief RA channel format with 32 signed floating point bits per channel.
  mux_image_format_R32A32_sfloat = (0x10B3 | (0x10DE << 16)),
  /// @brief RGB channel format with 16 bits of unsigned normalized storage.
  mux_image_format_R5G6B5_unorm_pack16 = (0x10B4 | (0x10D4 << 16)),
  /// @brief RGB channel format with 16 bits of unsigned normalized storage.
  mux_image_format_R5G5B5_unorm_pack16 = (0x10B4 | (0x10D5 << 16)),
  /// @brief RGB channel format with 32 bits of unsigned normalized storage.
  mux_image_format_R10G10B10_unorm_pack32 = (0x10B4 | (0x10D6 << 16)),
  /// @brief RGBA channel format with 8 signed normalized bits per channel.
  mux_image_format_R8G8B8A8_snorm = (0x10B5 | (0x10D0 << 16)),
  /// @brief RGBA channel format with 16 signed normalized bits per channel.
  mux_image_format_R16G16B16A16_snorm = (0x10B5 | (0x10D1 << 16)),
  /// @brief RGBA channel format with 8 unsigned normalized bits per channel.
  mux_image_format_R8G8B8A8_unorm = (0x10B5 | (0x10D2 << 16)),
  /// @brief RGBA channel format with 16 unsigned normalized bits per channel.
  mux_image_format_R16G16B16A16_unorm = (0x10B5 | (0x10D3 << 16)),
  /// @brief RGBA channel format with 8 signed integer bits per channel.
  mux_image_format_R8G8B8A8_sint = (0x10B5 | (0x10D7 << 16)),
  /// @brief RGBA channel format with 16 signed integer bits per channel.
  mux_image_format_R16G16B16A16_sint = (0x10B5 | (0x10D8 << 16)),
  /// @brief RGBA channel format with 32 signed integer bits per channel.
  mux_image_format_R32G32B32A32_sint = (0x10B5 | (0x10D9 << 16)),
  /// @brief RGBA channel format with 8 unsigned integer bits per channel.
  mux_image_format_R8G8B8A8_uint = (0x10B5 | (0x10DA << 16)),
  /// @brief RGBA channel format with 16 unsigned integer bits per channel.
  mux_image_format_R16G16B16A16_uint = (0x10B5 | (0x10DB << 16)),
  /// @brief RGBA channel format with 32 unsigned integer bits per channel.
  mux_image_format_R32G32B32A32_uint = (0x10B5 | (0x10DC << 16)),
  /// @brief RGBA channel format with 16 signed floating point bits per channel.
  mux_image_format_R16G16B16A16_sfloat = (0x10B5 | (0x10DD << 16)),
  /// @brief RGBA channel format with 32 signed floating point bits per channel.
  mux_image_format_R32G32B32A32_sfloat = (0x10B5 | (0x10DE << 16)),
  /// @brief BGRA channel format with 8 signed normalized bits per channel.
  mux_image_format_B8G8R8A8_snorm = (0x10B6 | (0x10D0 << 16)),
  /// @brief BGRA channel format with 8 unsigned normalized bits per channel.
  mux_image_format_B8G8R8A8_unorm = (0x10B6 | (0x10D2 << 16)),
  /// @brief BGRA channel format with 8 signed integer bits per channel.
  mux_image_format_B8G8R8A8_sint = (0x10B6 | (0x10D7 << 16)),
  /// @brief BGRA channel format with 8 unsigned integer bits per channel.
  mux_image_format_B8G8R8A8_uint = (0x10B6 | (0x10DA << 16)),
  /// @brief ARGB channel format with 8 signed normalized bits per channel.
  mux_image_format_A8R8G8B8_snorm = (0x10B7 | (0x10D0 << 16)),
  /// @brief ARGB channel format with 8 unsigned normalized bits per channel.
  mux_image_format_A8R8G8B8_unorm = (0x10B7 | (0x10D2 << 16)),
  /// @brief ARGB channel format with 8 signed integer bits per channel.
  mux_image_format_A8R8G8B8_sint = (0x10B7 | (0x10D7 << 16)),
  /// @brief ARGB channel format with 8 unsigned integer bits per channel.
  mux_image_format_A8R8G8B8_uint = (0x10B7 | (0x10DA << 16)),
  /// @brief Intensity channel format with 8 signed normalized bits per channel.
  mux_image_format_INTENSITY8_snorm = (0x10B8 | (0x10D0 << 16)),
  /// @brief Intensity channel format with 16 signed normalized bits per
  /// channel.
  mux_image_format_INTENSITY16_snorm = (0x10B8 | (0x10D1 << 16)),
  /// @brief Intensity channel format with 8 unsigned normalized bits per
  /// channel.
  mux_image_format_INTENSITY8_unorm = (0x10B8 | (0x10D2 << 16)),
  /// @brief Intensity channel format with 16 unsigned normalized bits per
  /// channel.
  mux_image_format_INTENSITY16_unorm = (0x10B8 | (0x10D3 << 16)),
  /// @brief Intensity channel format with 16 signed floating point bits per
  /// channel.
  mux_image_format_INTENSITY16_sfloat = (0x10B8 | (0x10DD << 16)),
  /// @brief Intensity channel format with 32 signed floating point bits per
  /// channel.
  mux_image_format_INTENSITY32_sfloat = (0x10B8 | (0x10DE << 16)),
  /// @brief Luminance channel format with 8 signed normalized bits per channel.
  mux_image_format_LUMINANCE8_snorm = (0x10B9 | (0x10D0 << 16)),
  /// @brief Luminance channel format with 16 signed normalized bits per
  /// channel.
  mux_image_format_LUMINANCE16_snorm = (0x10B9 | (0x10D1 << 16)),
  /// @brief Luminance channel format with 8 unsigned normalized bits per
  /// channel.
  mux_image_format_LUMINANCE8_unorm = (0x10B9 | (0x10D2 << 16)),
  /// @brief Luminance channel format with 16 unsigned normalized bits per
  /// channel.
  mux_image_format_LUMINANCE16_unorm = (0x10B9 | (0x10D3 << 16)),
  /// @brief Luminance channel format with 16 signed floating point bits per
  /// channel.
  mux_image_format_LUMINANCE16_sfloat = (0x10B9 | (0x10DD << 16)),
  /// @brief Luminance channel format with 32 signed floating point bits per
  /// channel.
  mux_image_format_LUMINANCE32_sfloat = (0x10B9 | (0x10DE << 16)),
  /// @brief Rx channel format with 8 signed normalized bits per channel.
  mux_image_format_R8x8_snorm = (0x10BA | (0x10D0 << 16)),
  /// @brief Rx channel format with 16 signed normalized bits per channel.
  mux_image_format_R16x16_snorm = (0x10BA | (0x10D1 << 16)),
  /// @brief Rx channel format with 8 unsigned normalized bits per channel.
  mux_image_format_R8x8_unorm = (0x10BA | (0x10D2 << 16)),
  /// @brief Rx channel format with 16 unsigned normalized bits per channel.
  mux_image_format_R16x16_unorm = (0x10BA | (0x10D3 << 16)),
  /// @brief Rx channel format with 8 signed integer bits per channel.
  mux_image_format_R8x8_sint = (0x10BA | (0x10D7 << 16)),
  /// @brief Rx channel format with 16 signed integer bits per channel.
  mux_image_format_R16x16_sint = (0x10BA | (0x10D8 << 16)),
  /// @brief Rx channel format with 32 signed integer bits per channel.
  mux_image_format_R32x32_sint = (0x10BA | (0x10D9 << 16)),
  /// @brief Rx channel format with 8 unsigned integer bits per channel.
  mux_image_format_R8x8_uint = (0x10BA | (0x10DA << 16)),
  /// @brief Rx channel format with 16 unsigned integer bits per channel.
  mux_image_format_R16x16_uint = (0x10BA | (0x10DB << 16)),
  /// @brief Rx channel format with 32 unsigned integer bits per channel.
  mux_image_format_R32x32_uint = (0x10BA | (0x10DC << 16)),
  /// @brief Rx channel format with 16 signed floating point bits per channel.
  mux_image_format_R16x16_sfloat = (0x10BA | (0x10DD << 16)),
  /// @brief Rx channel format with 32 signed floating point bits per channel.
  mux_image_format_R32x32_sfloat = (0x10BA | (0x10DE << 16)),
  /// @brief RGx channel format with 8 signed normalized bits per channel.
  mux_image_format_R8G8Bx_snorm = (0x10BB | (0x10D0 << 16)),
  /// @brief RGx channel format with 16 signed normalized bits per channel.
  mux_image_format_R16G16B16_snorm = (0x10BB | (0x10D1 << 16)),
  /// @brief RGx channel format with 8 unsigned normalized bits per channel.
  mux_image_format_R8G8Bx_unorm = (0x10BB | (0x10D2 << 16)),
  /// @brief RGx channel format with 16 unsigned normalized bits per channel.
  mux_image_format_R16G16B16_unorm = (0x10BB | (0x10D3 << 16)),
  /// @brief RGx channel format with 8 signed integer bits per channel.
  mux_image_format_R8G8Bx_sint = (0x10BB | (0x10D7 << 16)),
  /// @brief RGx channel format with 16 signed integer bits per channel.
  mux_image_format_R16G16B16_sint = (0x10BB | (0x10D8 << 16)),
  /// @brief RGx channel format with 32 signed integer bits per channel.
  mux_image_format_R32G32B32_sint = (0x10BB | (0x10D9 << 16)),
  /// @brief RGx channel format with 8 unsigned integer bits per channel.
  mux_image_format_R8G8Bx_uint = (0x10BB | (0x10DA << 16)),
  /// @brief RGx channel format with 16 unsigned integer bits per channel.
  mux_image_format_R16G16B16B16_uint = (0x10BB | (0x10DB << 16)),
  /// @brief RGx channel format with 32 unsigned integer bits per channel.
  mux_image_format_R32G32B32_uint = (0x10BB | (0x10DC << 16)),
  /// @brief RGx channel format with 16 signed floating point bits per channel.
  mux_image_format_R16G16B16_sfloat = (0x10BB | (0x10DD << 16)),
  /// @brief RGx channel format with 32 signed floating point bits per channel.
  mux_image_format_R32G32B32_sfloat = (0x10BB | (0x10DE << 16)),
  /// @brief RGBx channel format with 16 bits of unsigned normalized storage.
  mux_image_format_R5G6B5x0_unorm_pack16 = (0x10BC | (0x10D4 << 16)),
  /// @brief RGBx channel format with 16 bits of unsigned normalized storage.
  mux_image_format_R5G5B5x1_unorm_pack16 = (0x10BC | (0x10D5 << 16)),
  /// @brief RGBx channel format with 32 bits of unsigned normalized storage.
  mux_image_format_R10G10B10x2_unorm_pack32 = (0x10BC | (0x10D6 << 16))
} mux_image_format_e;

/// @brief All possible addressing types that can be used.
///
/// Used to specify what modes of addressing are supported by the platform.
enum mux_address_type_e {
  /// @brief Logical addressing.
  mux_address_type_logical = 0,
  /// @brief 32-bit wide pointer addressing.
  mux_address_type_bits32 = 1,
  /// @brief 64-bit wide pointer addressing.
  mux_address_type_bits64 = 2
};

/// @brief Bitfield of all possible addressing capabilities.
///
/// Each Mux device info struct has a member which denotes the addressing
/// capabilities of that device, as a bitfield of the following enum.
///
/// @see mux_device_info_s::source_capabilities
enum mux_address_capabilities_e {
  /// @brief Logical addressing supported.
  mux_address_capabilities_logical = 0x1 << mux_address_type_logical,
  /// @brief 32-bit wide pointer addressing supported.
  mux_address_capabilities_bits32 = 0x1 << mux_address_type_bits32,
  /// @brief 64-bit wide pointer addressing supported.
  mux_address_capabilities_bits64 = 0x1 << mux_address_type_bits64
};

/// @brief Bitfield of all possible caching capabilities.
///
/// Each Mux device struct has a member which denotes the caching capabilities
/// of that device, as a bitfield of the following enum.
enum mux_cache_capabilities_e {
  /// @brief Read caching supported.
  mux_cache_capabilities_read = 0x1,
  /// @brief Write caching supported.
  mux_cache_capabilities_write = 0x2
};

/// @brief Bitfield of all possible floating point capabilities.
///
/// Each Mux device struct has a member which denotes the floating point
/// capabilities of that device, as a bitfield of the following enum.
enum mux_floating_point_capabilities_e {
  /// @brief Denormals supported.
  mux_floating_point_capabilities_denorm = 0x1,
  /// @brief INF and NaN are supported.
  mux_floating_point_capabilities_inf_nan = 0x2,
  /// @brief Round to nearest even supported.
  mux_floating_point_capabilities_rte = 0x4,
  /// @brief Round to zero supported.
  mux_floating_point_capabilities_rtz = 0x8,
  /// @brief Round to positive infinity supported.
  mux_floating_point_capabilities_rtp = 0x10,
  /// @brief Round to negative infinity supported.
  mux_floating_point_capabilities_rtn = 0x20,
  /// @brief Fused multiply add supported.
  mux_floating_point_capabilities_fma = 0x40,
  /// @brief Floating point operations are written in software.
  mux_floating_point_capabilities_soft = 0x80,
  /// @brief Binary format conforms to the IEEE-754 specification.
  mux_floating_point_capabilities_full = 0x100
};

/// @brief Bitfield of all possible integer capabilities.
///
/// Each Mux device struct has a member which denotes the integer capabilities
/// of that device, as a bitfield of the following enum.
enum mux_integer_capabilities_e {
  /// @brief 8 bit integer support
  mux_integer_capabilities_8bit = 0x1,
  /// @brief 16 bit integer support
  mux_integer_capabilities_16bit = 0x2,
  /// @brief 32 bit integer support
  mux_integer_capabilities_32bit = 0x4,
  /// @brief 64 bit integer support
  mux_integer_capabilities_64bit = 0x8
};

/// @brief Bitfield of all possible custom buffer capabilities.
///
/// Each Mux device struct has a member which denotes the custom buffer
/// capabilities of that device, as a bitfield of the following enum.
enum mux_custom_buffer_capabilities_e {
  /// @brief Custom buffer support for setting `data` and `size` in
  /// mux_descriptor_info_custom_buffer_s.
  mux_custom_buffer_capabilities_data = 0x1,
  /// @brief Custom buffer support for setting `address_space` in
  /// mux_descriptor_info_custom_buffer_s.
  mux_custom_buffer_capabilities_address_space = 0x2
};

/// @brief All possible endiannesses.
enum mux_endianness_e {
  /// @brief Little endian device
  mux_endianness_little = 0x1,
  /// @brief Big endian device
  mux_endianness_big = 0x2
};

/// @brief All possible shared local memory types.
enum mux_shared_local_memory_type_e {
  /// The shared local memory used by the device is truely that - physical
  /// memory on the device.
  mux_shared_local_memory_physical = 0,
  /// The shared local memory used by the device is virtual - its actually using
  /// the global memory available to the device to pretend that it has shared
  /// local memory support.
  mux_shared_local_memory_virtual = 1
};

/// @brief All possible descriptor types.
///
/// Used to identify which type a descriptor is.
///
/// @see mux_descriptor_info_s::_type
enum mux_descriptor_info_type_e {
  /// @brief Descriptor is a buffer.
  mux_descriptor_info_type_buffer = 0,
  /// @brief Descriptor is an image.
  mux_descriptor_info_type_image = 1,
  /// @brief Descriptor is a sampler.
  mux_descriptor_info_type_sampler = 2,
  /// @brief Descriptor is plain-old-data.
  mux_descriptor_info_type_plain_old_data = 3,
  /// @brief Descriptor is a shared local buffer.
  mux_descriptor_info_type_shared_local_buffer = 4,
  /// @brief Descriptor is a null buffer.
  mux_descriptor_info_type_null_buffer = 5,
  /// @brief Descriptor is a custom buffer.
  mux_descriptor_info_type_custom_buffer = 6
};

/// @brief All possible addressing modes describing how to handle out of range
/// image coordinates.
typedef enum mux_address_mode_e {
  /// @brief Image coordinates must reside within the image dimensions.
  mux_address_mode_none,
  /// @brief Out of range image coordinates are wrapped around to a valid range.
  mux_address_mode_repeat,
  /// @brief Out of range image coordinates are mirrored along image boundaries.
  mux_address_mode_repeat_mirror,
  /// @brief Out of range image coordinates are clamped to the border colour.
  mux_address_mode_clamp,
  /// @brief Out of range image coordinates are clamped to the image extent.
  mux_address_mode_clamp_edge
} mux_address_mode_e;

/// @brief All possible filter modes for image sampling.
typedef enum mux_filter_mode_e {
  /// @brief Sample the nearest pixel to the specified coordinate.
  mux_filter_mode_nearest,
  /// @brief Sample and blend the neighbouring pixels to the specified
  /// coordinate.
  mux_filter_mode_linear
} mux_filter_mode_e;

/// @brief Accepted as `query_type` parameter to muxCreateQueryPool.
///
/// The type of a query pool specifies the data that can be stored within it.
typedef enum mux_query_type_e {
  /// @brief Query the command duration with a start and end time stamp, the
  /// `start` and `end` timestamps **must** be CPU timestamps which **may**
  /// require interpolation of device timestamps, only a single command duration
  /// query **shall** be enabled in a command buffer at one time.
  mux_query_type_duration,
  /// @brief Query the values of an enabled set of hardware counters.
  mux_query_type_counter
} mux_query_type_e;

/// @brief All possible query counter storage types.
typedef enum mux_query_counter_storage_e {
  /// @brief Query counter storage is `int32_t`.
  mux_query_counter_result_type_int32,
  /// @brief Query counter storage is `int64_t`.
  mux_query_counter_result_type_int64,
  /// @brief Query counter storage is `uint32_t`.
  mux_query_counter_result_type_uint32,
  /// @brief Query counter storage is `uint64_t`.
  mux_query_counter_result_type_uint64,
  /// @brief Query counter storage is `float`.
  mux_query_counter_result_type_float32,
  /// @brief Query counter storage is `double`.
  mux_query_counter_result_type_float64
} mux_query_counter_storage_e;

/// @brief All possible query counter unit types.
typedef enum mux_query_counter_unit_e {
  /// @brief The counter unit is a generic value.
  mux_query_counter_unit_generic,
  /// @brief The counter unit is a percentage.
  mux_query_counter_unit_percentage,
  /// @brief The counter unit is nanoseconds.
  mux_query_counter_unit_nanoseconds,
  /// @brief The counter unit is bytes.
  mux_query_counter_unit_bytes,
  /// @brief The counter unit is bytes per second.
  mux_query_counter_unit_bytes_per_second,
  /// @brief The counter unit is degrees kelvin.
  mux_query_counter_unit_kelvin,
  /// @brief The counter unit is watts.
  mux_query_counter_unit_watts,
  /// @brief The counter unit is volts.
  mux_query_counter_unit_volts,
  /// @brief The counter unit is amperes.
  mux_query_counter_unit_amps,
  /// @brief The counter unit is hertz.
  mux_query_counter_unit_hertz,
  /// @brief The counter unit is cycles.
  mux_query_counter_unit_cycles
} mux_query_counter_unit_e;

/// @brief All possible whole function vectorization status codes for a kernel.
typedef enum mux_wfv_status_e {
  /// @brief Whole function vectorization has not been performed.
  mux_wfv_status_none,
  /// @brief An error was encountered while trying to perform whole function
  /// vectorization.
  mux_wfv_status_error,
  /// @brief Whole function vectorization has been performed successfully.
  mux_wfv_status_success
} mux_wfv_status_e;

/// @brief Command buffer user callback function type.
///
/// A parameter type of muxCommandUserCallback, defining the signature of
/// callback functors.
typedef void (*mux_command_user_callback_t)(mux_queue_t queue,
                                            mux_command_buffer_t command_buffer,
                                            void *const user_data);

/// @brief Gets Mux devices' information.
///
/// @param[in] device_types A bitfield of `::mux_device_type_e` values to return
/// in `out_device_infos`.
/// @param[in] device_infos_length The length of out_device_infos. Must be 0, if
/// out_devices is null.
/// @param[out] out_device_infos Array of information for devices Mux knows
/// about, or null if an error occurred. Can be null, if out_device_infos_length
/// is non-null.
/// @param[out] out_device_infos_length The total number of devices for which we
/// are returning information, or 0 if an error occurred. Can be null, if
/// out_device_infos is non-null.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxGetDeviceInfos(uint32_t device_types,
                               uint64_t device_infos_length,
                               mux_device_info_t *out_device_infos,
                               uint64_t *out_device_infos_length);

/// @param[in] devices_length The length of out_devices. Must be 0, if
/// out_devices is null.
/// @param[in] device_infos Array of device information determining which
/// devices to create.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_devices Array of devices Mux knows about, or null if an
/// error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCreateDevices(uint64_t devices_length,
                              mux_device_info_t *device_infos,
                              mux_allocator_info_t allocator_info,
                              mux_device_t *out_devices);

/// @brief Destroy a device.
///
/// This function allows Mux to call into our partner's code to destroy a device
/// that was created previously by the partner.
///
/// @param[in] device A Mux device to destroy.
/// @param[in] allocator_info Allocator information.
void muxDestroyDevice(mux_device_t device, mux_allocator_info_t allocator_info);

/// @brief Allocate Mux device memory to be bound to a buffer or image.
///
/// This function uses a Mux device to allocate device memory which can later be
/// bound to buffers and images, providing physical memory backing, with the
/// ::muxBindBufferMemory and ::muxBindImageMemory functions.
///
/// @param[in] device A Mux device.
/// @param[in] size The size in bytes of memory to allocate.
/// @param[in] heap Value of a single set bit in the
/// `::mux_memory_requirements_s::supported_heaps` bitfield of the buffer or
/// image the device memory is being allocated for. Passing the value of `1` to
/// `heap` must result in a successful allocation, `heap` must not be `0`.
/// @param[in] memory_properties Bitfield of memory properties this allocation
/// should support, values from mux_memory_property_e.
/// @param[in] allocation_type The type of allocation.
/// @param[in] alignment Minimum alignment in bytes for the requested
/// allocation.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_memory The created memory, or uninitialized if an error
/// occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxAllocateMemory(mux_device_t device, size_t size, uint32_t heap,
                               uint32_t memory_properties,
                               mux_allocation_type_e allocation_type,
                               uint32_t alignment,
                               mux_allocator_info_t allocator_info,
                               mux_memory_t *out_memory);

/// @brief Assigns Mux device visible memory from pre-allocated host side
/// memory.
///
/// This function takes a pointer to pre-allocated host memory and binds it to
/// device visible memory, which is cache coherent with the host allocation.
/// Entry point is optional and must return mux_error_feature_unsupported if
/// device doesn't support mux_allocation_capabilities_cached_host.
///
/// @param[in] device A Mux device.
/// @param[in] size The size in bytes of allocated memory.
/// @param[in] host_pointer Pointer to pre-allocated host addressable memory,
/// must not be null.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_memory The created memory, or uninitialized if an error
/// occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
///
/// @see mux_device_info_s::allocation_capabilities
mux_result_t muxCreateMemoryFromHost(mux_device_t device, size_t size,
                                     void *host_pointer,
                                     mux_allocator_info_t allocator_info,
                                     mux_memory_t *out_memory);

/// @brief Free Mux device memory after use has finished.
///
/// This function uses the Mux device used for allocation with either
/// ::muxAllocateMemory or ::muxCreateMemoryFromHost to deallocate the device
/// memory.
///
/// @param[in] device The Mux device the memory was allocated for.
/// @param[in] memory The Mux device memory to be freed.
/// @param[in] allocator_info Allocator information.
void muxFreeMemory(mux_device_t device, mux_memory_t memory,
                   mux_allocator_info_t allocator_info);

/// @brief Map Mux device memory to a host address.
///
/// @param[in] device The device where the device memory is allocated.
/// @param[in] memory The memory to be mapped to a host accessible memory
/// address.
/// @param[in] offset Offset in bytes into the device memory to map to host
/// addressable memory.
/// @param[in] size Size in bytes of device memory to map to host addressable
/// memory.
/// @param[out] out_data Pointer to returned mapped host address, must not be
/// null.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxMapMemory(mux_device_t device, mux_memory_t memory,
                          uint64_t offset, uint64_t size, void **out_data);

/// @brief Unmap a mapped device memory.
///
/// @param[in] device The device where the device memory is allocated.
/// @param[in] memory The device memory to unmap.
mux_result_t muxUnmapMemory(mux_device_t device, mux_memory_t memory);

/// @brief Explicitly update device memory with data residing in host memory.
///
/// muxFlushMappedMemoryToDevice is intended to be used with mux_memory_ts
/// allocated with the mux_memory_property_host_cached flag set. It updates
/// device memory with the content currently residing in host memory.
///
/// @param[in] device The device where the memory is allocated.
/// @param[in] memory The device memory to be flushed.
/// @param[in] offset The offset in bytes into the device memory to begin the
/// range.
/// @param[in] size The size in bytes of the range to flush.
///
/// @return Returns mux_success on success or mux_error_invalid_value when; @p
/// device is not a valid mux_device_t, @p memory is not a valid mux_memory_t,
/// @p offset combined with @p size is greater than @p memory size.
mux_result_t muxFlushMappedMemoryToDevice(mux_device_t device,
                                          mux_memory_t memory, uint64_t offset,
                                          uint64_t size);

/// @brief Explicitly update host memory with data residing in device memory.
///
/// muxFlushMappedMemoryFromDevice is intended to be used with mux_memory_ts
/// allocated with the mux_memory_property_host_cached flag set. It updates host
/// memory with the content currently residing in device memory.
///
/// @param[in] device The device where the memory is allocated.
/// @param[in] memory The device memory to be flushed.
/// @param[in] offset The offset in bytes into the device memory to begin the
/// range.
/// @param[in] size The size in bytes of the range to flush.
///
/// @return Returns mux_success on success or mux_error_invalid_value when; @p
/// device is not a valid mux_device_t, @p memory is not a valid mux_memory_t,
/// @p offset combined with @p size is greater than @p memory size.
mux_result_t muxFlushMappedMemoryFromDevice(mux_device_t device,
                                            mux_memory_t memory,
                                            uint64_t offset, uint64_t size);

/// @brief Create buffer memory.
///
/// The function uses a Mux device, and is used to call into the Mux device code
/// to create a buffer of a specific type and size.
///
/// @param[in] device A Mux device.
/// @param[in] size The size (in bytes) of buffer requested.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_buffer The created buffer, or uninitialized if an error
/// occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCreateBuffer(mux_device_t device, size_t size,
                             mux_allocator_info_t allocator_info,
                             mux_buffer_t *out_buffer);

/// @brief Destroy a buffer.
///
/// This function allows Mux to call into our partner's code to destroy a buffer
/// that was created previously by the partner.
///
/// @param[in] device The Mux device the buffer was created with.
/// @param[in] buffer The buffer to destroy.
/// @param[in] allocator_info Allocator information.
void muxDestroyBuffer(mux_device_t device, mux_buffer_t buffer,
                      mux_allocator_info_t allocator_info);

/// @brief Bind Mux device memory to the Mux buffer.
///
/// Bind device memory to a buffer providing physical backing so it can be used
/// when processing a ::mux_command_buffer_t.
///
/// @param[in] device The device on which the memory resides.
/// @param[in] memory The device memory to be bound to the buffer.
/// @param[in] buffer The buffer to which the device memory is to be bound.
/// @param[in] offset The offset into device memory to bind to the buffer.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxBindBufferMemory(mux_device_t device, mux_memory_t memory,
                                 mux_buffer_t buffer, uint64_t offset);

/// @brief Create an image.
///
/// The function uses a Mux device to create an image with specific type,
/// format, and dimensions. Newly created images are by default in the @p
/// ::mux_image_tiling_linear mode.
///
/// @param[in] device A Mux device.
/// @param[in] type The type of image to create.
/// @param[in] format The pixel data format of the image to create.
/// @param[in] width The width of the image in pixels, must be greater than one
/// and less than max width.
/// @param[in] height The height of the image in pixels, must be zero for 1D
/// images, must be greater than one and less than max height for 2D and 3D
/// images.
/// @param[in] depth The depth of the image in pixels, must be zero for 1D and
/// 2D images, must be greater than one and less than max depth for 3D images.
/// @param[in] array_layers The number of layers in an image array, must be 0
/// for non image arrays, must be and less than max array layers for image
/// arrays.
/// @param[in] row_size The size of an image row in bytes.
/// @param[in] slice_size The size on an image slice in bytes.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_image The created image, or null if an error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCreateImage(mux_device_t device, mux_image_type_e type,
                            mux_image_format_e format, uint32_t width,
                            uint32_t height, uint32_t depth,
                            uint32_t array_layers, uint64_t row_size,
                            uint64_t slice_size,
                            mux_allocator_info_t allocator_info,
                            mux_image_t *out_image);

/// @brief Destroy an image.
///
/// The function allows Mux to call into client code to destroy an image that
/// was previously created.
///
/// @param[in] device The device the image was created with.
/// @param[in] image The image to destroy.
/// @param[in] allocator_info Allocator information.
void muxDestroyImage(mux_device_t device, mux_image_t image,
                     mux_allocator_info_t allocator_info);

/// @brief Bind Mux device memory to the Mux image.
///
/// Bind device memory to an image providing physical backing so it can be used
/// when processing a ::mux_command_buffer_t.
///
/// @param[in] device The device on which the memory resides.
/// @param[in] memory The device memory to be bound to the image.
/// @param[in] image The image to which the device memory is to be bound.
/// @param[in] offset The offset into device memory to bind the image.
///
/// @return Return mux_success on success, or a mux_error_* if an error
/// occurred.
mux_result_t muxBindImageMemory(mux_device_t device, mux_memory_t memory,
                                mux_image_t image, uint64_t offset);

/// @brief Query the Mux device for a list of supported image formats.
///
/// @param[in] device The device to query for supported image formats.
/// @param[in] image_type The type of the image.
/// @param[in] allocation_type The required allocation capabilities of the
/// image.
/// @param[in] count The element count of the @p out_formats array, must be
/// greater than zero if @p out_formats is not null and zero otherwise.
/// @param[out] out_formats Return the list of supported formats, may be null.
/// Storage must be an array of @p out_count elements.
/// @param[out] out_count Return the number of supported formats, may be null.
///
/// @return Return mux_success on success, or a mux_error_* if an error
/// occurred.
mux_result_t muxGetSupportedImageFormats(mux_device_t device,
                                         mux_image_type_e image_type,
                                         mux_allocation_type_e allocation_type,
                                         uint32_t count,
                                         mux_image_format_e *out_formats,
                                         uint32_t *out_count);

/// @brief Get the list of supported query counters for a device's queue type.
///
/// @param[in] device A Mux device.
/// @param[in] queue_type The type of queue to get the supported query counters
/// list for.
/// @param[in] count The element count of the `out_counters` and
/// `out_descriptions` arrays, **must** be greater than zero if `out_counters`
/// is not null and zero otherwise.
/// @param[out] out_counters Return the list of supported query counters,
/// **may** be null. Storage **must** be an array of `count` elements.
/// @param[out] out_descriptions Return the list of descriptions of support
/// query counters, **may** be null. Storage **must** be an array of `count`
/// elements.
/// @param[out] out_count Return the total count of supported query counters.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxGetSupportedQueryCounters(
    mux_device_t device, mux_queue_type_e queue_type, uint32_t count,
    mux_query_counter_t *out_counters,
    mux_query_counter_description_t *out_descriptions, uint32_t *out_count);

/// @brief Get a queue from the owning device.
///
/// The function uses a Mux device, and is used to call into the Mux code to
/// create a queue.
///
/// @param[in] device A Mux device.
/// @param[in] queue_type The type of queue we want.
/// @param[in] queue_index The index of the queue_type we want to get from the
/// device.
/// @param[out] out_queue The created queue, or uninitialized if an error
/// occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxGetQueue(mux_device_t device, mux_queue_type_e queue_type,
                         uint32_t queue_index, mux_queue_t *out_queue);

/// @brief Create a fence.
///
/// The function uses Mux device, and is used to call into the Mux code to
/// create a fence.
///
/// @param[in] device A Mux device.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_fence The created fence, or uninitialized if an error
/// occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCreateFence(mux_device_t device,
                            mux_allocator_info_t allocator_info,
                            mux_fence_t *out_fence);

/// @brief Destroy a fence.
///
/// This hook allows Mux to call into our partner's code to destroy a fence that
/// was created previously by the partner.
///
/// @param[in] device The Mux device the fence was created with.
/// @param[in] fence The fence to destroy.
/// @param[in] allocator_info Allocator information.
void muxDestroyFence(mux_device_t device, mux_fence_t fence,
                     mux_allocator_info_t allocator_info);

/// @brief Reset a fence such that it has no previous signalled state.
///
/// @param[in] fence The fence to reset.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxResetFence(mux_fence_t fence);

/// @brief Create a semaphore.
///
/// The function uses Mux device, and is used to call into the Mux code to
/// create a semaphore.
///
/// @param[in] device A Mux device.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_semaphore The created semaphore, or uninitialized if an
/// error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCreateSemaphore(mux_device_t device,
                                mux_allocator_info_t allocator_info,
                                mux_semaphore_t *out_semaphore);

/// @brief Destroy a semaphore.
///
/// This hook allows Mux to call into our partner's code to destroy a semaphore
/// that was created previously by the partner.
///
/// @param[in] device The Mux device the semaphore was created with.
/// @param[in] semaphore The semaphore to destroy.
/// @param[in] allocator_info Allocator information.
void muxDestroySemaphore(mux_device_t device, mux_semaphore_t semaphore,
                         mux_allocator_info_t allocator_info);

/// @brief Reset a semaphore such that it has no previous signalled state.
///
/// @param[in] semaphore The semaphore to reset.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxResetSemaphore(mux_semaphore_t semaphore);

/// @brief Create a command buffer.
///
/// This function allows Mux to call into our partner's code to create a command
/// buffer. Command buffers are a construct whereby we can encapsulate a group
/// of commands for execution on a device.
///
/// @param[in] device A Mux device.
/// @param[in] callback_info User provided callback, which **may** be null, can
/// be used by the implementation to provide detailed messages about command
/// buffer execution to the user.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_command_buffer The newly created command buffer, or
/// uninitialized if an error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCreateCommandBuffer(mux_device_t device,
                                    mux_callback_info_t callback_info,
                                    mux_allocator_info_t allocator_info,
                                    mux_command_buffer_t *out_command_buffer);

/// @brief Finalize a command buffer.
///
/// This function allows Mux to call into our partner's code to finalize a
/// command buffer. Finalized command buffers are in an immutable state and
/// cannot have further commands pushed to them.
///
/// @param[in] command_buffer A Mux command buffer.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxFinalizeCommandBuffer(mux_command_buffer_t command_buffer);

/// @brief Clone a command buffer.
///
/// This function allows Mux to call into our partner's code to clone a command
/// buffer.
///
/// @param[in] device A Mux device.
/// @param[in] allocator_info Allocator information.
/// @param[in] command_buffer A Mux command buffer to clone.
/// @param[out] out_command_buffer The newly created command buffer, or
/// uninitialized if an error occurred.
///
/// @return mux_success, a mux_error_* if an error occurred or
/// mux_error_feature_unsupported if cloning command buffers is not supported by
/// the device.
mux_result_t muxCloneCommandBuffer(mux_device_t device,
                                   mux_allocator_info_t allocator_info,
                                   mux_command_buffer_t command_buffer,
                                   mux_command_buffer_t *out_command_buffer);

/// @brief Destroy a command buffer.
///
/// This hook allows Mux to call into our partner's code to destroy a command
/// buffer that was created previously by the partner.
///
/// @param[in] device The Mux device the command buffer was created with.
/// @param[in] command_buffer The command buffer to destroy.
/// @param[in] allocator_info Allocator information.
void muxDestroyCommandBuffer(mux_device_t device,
                             mux_command_buffer_t command_buffer,
                             mux_allocator_info_t allocator_info);

/// @brief Load a binary into an executable container.
///
/// This function takes a precompiled binary and turns it into an executable.
///
/// @param[in] device The Mux device to create the executable with.
/// @param[in] binary The source binary data.
/// @param[in] binary_length The length of the source binary (in bytes).
/// @param[in] allocator_info Allocator information.
/// @param[out] out_executable The newly created executable, or uninitialized if
/// an error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCreateExecutable(mux_device_t device, const void *binary,
                                 uint64_t binary_length,
                                 mux_allocator_info_t allocator_info,
                                 mux_executable_t *out_executable);

/// @brief Destroy an executable.
///
/// This hook allows Mux to call into our partner's code to destroy an
/// executable that was created previously by the partner.
///
/// @param[in] device The Mux device the executable was created with.
/// @param[in] executable The executable to destroy.
/// @param[in] allocator_info Allocator information.
void muxDestroyExecutable(mux_device_t device, mux_executable_t executable,
                          mux_allocator_info_t allocator_info);

/// @brief Create a kernel from an executable.
///
/// This function creates an object that represents a kernel function inside an
/// executable.
///
/// @param[in] device The Mux device to create the kernel with.
/// @param[in] executable A previously created Mux executable.
/// @param[in] name The name of the kernel we want to create.
/// @param[in] name_length The length (in bytes) of @p name.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_kernel The newly created kernel, or uninitialized if an
/// error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCreateKernel(mux_device_t device, mux_executable_t executable,
                             const char *name, uint64_t name_length,
                             mux_allocator_info_t allocator_info,
                             mux_kernel_t *out_kernel);

/// @brief Create a kernel from a built-in kernel name provided by the device.
///
/// This function creates an object that represents a kernel which is provided
/// by the device itself, rather than contained within a binary contained within
/// an executable.
///
/// @param[in] device The Mux device to create the kernel with.
/// @param[in] name The name of the kernel we want to create.
/// @param[in] name_length The length (in bytes) of @p name.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_kernel The newly created kernel, or uninitialized if an
/// error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCreateBuiltInKernel(mux_device_t device, const char *name,
                                    uint64_t name_length,
                                    mux_allocator_info_t allocator_info,
                                    mux_kernel_t *out_kernel);

/// @brief Query the maximum number of sub-groups that this kernel supports for
/// each work-group.
///
/// @param[in] kernel The Mux kernel to query the max number of subgroups for.
/// @param[out] out_max_num_sub_groups The maximum number of sub-groups this
/// kernel supports.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxQueryMaxNumSubGroups(mux_kernel_t kernel,
                                     size_t *out_max_num_sub_groups);

/// @brief Query the local size that would give the requested number of
/// sub-groups.
///
/// This function queries a kernel for the local size that would give the
/// requested number of sub-groups. It must return 0 if no local size would give
/// the requested number of sub-groups.
///
/// @param[in] kernel The Mux kernel to query the local size for.
/// @param[in] sub_group_count The requested number of sub-groups.
/// @param[out] out_local_size_x The local size in the x dimension which would
/// give the requested number of sub-groups.
/// @param[out] out_local_size_y The local size in the y dimension which would
/// give the requested number of sub-groups.
/// @param[out] out_local_size_z The local size in the z dimension which would
/// give the requested number of sub-groups.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxQueryLocalSizeForSubGroupCount(mux_kernel_t kernel,
                                               size_t sub_group_count,
                                               size_t *out_local_size_x,
                                               size_t *out_local_size_y,
                                               size_t *out_local_size_z);

/// @brief Query the sub-group size for the given local size.
///
/// This function queries a kernel for maximum sub-group size that would exist
/// in a kernel enqueued with the local work-group size passed as parameters. A
/// kernel enqueue **may** include one sub-group with a smaller size when the
/// sub-group size doesn't evenly divide the local size.
///
/// @param[in] kernel The Mux kernel to query the sub-group size for.
/// @param[in] local_size_x The local size in the x dimension for which we wish
/// to query sub-group size.
/// @param[in] local_size_y The local size in the y dimension for which we wish
/// to query sub-group size.
/// @param[in] local_size_z The local size in the z dimension for which we wish
/// to query sub-group size.
/// @param[out] out_sub_group_size The maximum sub-group sub-group size for the
/// queried local size.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxQuerySubGroupSizeForLocalSize(mux_kernel_t kernel,
                                              size_t local_size_x,
                                              size_t local_size_y,
                                              size_t local_size_z,
                                              size_t *out_sub_group_size);

/// @brief Query the whole function vectorization status and dimension work
/// widths for the given local size.
///
/// This function queries a kernel for the whole function vectorization status
/// and dimension work widths that would be applicable for a kernel enqueued
/// with the local work-group size passed as parameters. The number of
/// work-items executed per kernel invocation **shall** be equal to the
/// dimension work widths returned from this function.
///
/// @param[in] kernel The Mux kernel to query the WFV info for.
/// @param[in] local_size_x The local size in the x dimension for which we wish
/// to query WFV info.
/// @param[in] local_size_y The local size in the y dimension for which we wish
/// to query WFV info.
/// @param[in] local_size_z The local size in the z dimension for which we wish
/// to query WFV info.
/// @param[out] out_wfv_status The status of whole function vectorization for
/// the queried local size.
/// @param[out] out_work_width_x The work width in the x dimension for the
/// queried local size.
/// @param[out] out_work_width_y The work width in the y dimension for the
/// queried local size.
/// @param[out] out_work_width_z The work width in the z dimension for the
/// queried local size.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxQueryWFVInfoForLocalSize(
    mux_kernel_t kernel, size_t local_size_x, size_t local_size_y,
    size_t local_size_z, mux_wfv_status_e *out_wfv_status,
    size_t *out_work_width_x, size_t *out_work_width_y,
    size_t *out_work_width_z);

/// @brief Destroy a kernel.
///
/// This hook allows Mux to call into our partner's code to destroy a kernel
/// that was created previously by the partner.
///
/// @param[in] device The Mux device the kernel was created with.
/// @param[in] kernel The kernel to destroy.
/// @param[in] allocator_info Allocator information.
void muxDestroyKernel(mux_device_t device, mux_kernel_t kernel,
                      mux_allocator_info_t allocator_info);

/// @brief Create a query pool.
///
/// This function creates a query pool into which data can be stored about the
/// runtime behavior of commands on a queue.
///
/// @param[in] queue The Mux queue to create the query pool with.
/// @param[in] query_type The type of query pool to create (one of the values in
/// the enum mux_query_type_e).
/// @param[in] query_count The number of queries to allocate storage for.
/// @param[in] query_counter_configs Array of query counter configuration data
/// with length `query_count`, **must** be provided when `query_type` is
/// `mux_query_type_counter`, otherwise **must** be NULL.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_query_pool The newly created query pool, or uninitialized if
/// an error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCreateQueryPool(
    mux_queue_t queue, mux_query_type_e query_type, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs,
    mux_allocator_info_t allocator_info, mux_query_pool_t *out_query_pool);

/// @brief Destroy a query pool.
///
/// This hook allows Mux to call into our partner's code to destroy a query pool
/// that was created previously by the partner.
///
/// @param[in] queue The Mux queue the query pool was created with.
/// @param[in] query_pool The Mux query pool to destroy.
/// @param[in] allocator_info Allocator information.
void muxDestroyQueryPool(mux_queue_t queue, mux_query_pool_t query_pool,
                         mux_allocator_info_t allocator_info);

/// @brief Get the number of passes required for valid query counter results.
///
/// @param[in] queue A Mux queue.
/// @param[in] query_count The number of elements in the array pointed to by
/// `query_info`.
/// @param[in] query_counter_configs Array of query counter configuration data
/// with length `query_count`, **must** be provided when `query_type` is
/// `mux_query_type_counter`, otherwise **must** be NULL.
/// @param[out] out_pass_count Return the number of passes required to produce
/// valid results for the given list of query counters.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxGetQueryCounterRequiredPasses(
    mux_queue_t queue, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs,
    uint32_t *out_pass_count);

/// @brief Get query results previously written to the query pool.
///
/// This function copies the requested query pool results previously written
/// during runtime on the Mux queue into the user provided storage.
///
/// @param[in] queue The Mux queue the query pool was created with.
/// @param[in] query_pool The Mux query pool to get the results from.
/// @param[in] query_index The query index of the first query to get the result
/// of.
/// @param[in] query_count The number of queries to get the results of.
/// @param[in] size The size in bytes of the memory pointed to by `data`.
/// @param[out] data A pointer to the memory to write the query results into.
/// @param[in] stride The stride in bytes between query results to be written
/// into `data`.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxGetQueryPoolResults(mux_queue_t queue,
                                    mux_query_pool_t query_pool,
                                    uint32_t query_index, uint32_t query_count,
                                    size_t size, void *data, size_t stride);

/// @brief Push a read buffer command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the read command to.
/// @param[in] buffer The buffer to read from.
/// @param[in] offset The offset (in bytes) into the buffer object to read.
/// @param[out] host_pointer The host pointer to write to.
/// @param[in] size The size (in bytes) of the buffer object to read.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandReadBuffer(mux_command_buffer_t command_buffer,
                                  mux_buffer_t buffer, uint64_t offset,
                                  void *host_pointer, uint64_t size,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point);

/// @brief Push a read buffer regions command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the command to.
/// @param[in] buffer The buffer to read from.
/// @param[out] host_pointer The host pointer to write to.
/// @param[in] regions The regions to read from buffer to host_pointer.
/// @param[in] regions_length The length of regions.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandReadBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer,
    void *host_pointer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a write buffer command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the write command to.
/// @param[in] buffer The buffer to write to.
/// @param[in] offset The offset (in bytes) into the buffer object to write.
/// @param[in] host_pointer The host pointer to read from.
/// @param[in] size The size (in bytes) of the buffer object to write.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandWriteBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t buffer, uint64_t offset,
                                   const void *host_pointer, uint64_t size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point);

/// @brief Push a write buffer regions command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the command to.
/// @param[in] buffer The buffer to write to.
/// @param[in] host_pointer The host pointer to read from.
/// @param[in] regions The regions to read from host_pointer to buffer.
/// @param[in] regions_length The length of regions.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandWriteBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer,
    const void *host_pointer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a copy buffer command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the copy command to.
/// @param[in] src_buffer The source buffer to copy from.
/// @param[in] src_offset The offset (in bytes) into the source buffer to copy.
/// @param[in] dst_buffer The destination buffer to copy to.
/// @param[in] dst_offset The offset (in bytes) into the destination buffer to
/// copy.
/// @param[in] size The size (in bytes) to copy.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandCopyBuffer(mux_command_buffer_t command_buffer,
                                  mux_buffer_t src_buffer, uint64_t src_offset,
                                  mux_buffer_t dst_buffer, uint64_t dst_offset,
                                  uint64_t size,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point);

/// @brief Push a copy buffer regions command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the copy command to.
/// @param[in] src_buffer The source buffer to copy from.
/// @param[in] dst_buffer The destination buffer to copy to.
/// @param[in] regions The regions to copy from src to dst.
/// @param[in] regions_length The length of regions.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandCopyBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_buffer_t dst_buffer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a write buffer command to the command buffer.
///
/// @param[in] command_buffer The command buffer push the fill command to.
/// @param[in] buffer The buffer to fill.
/// @param[in] offset The offset (in bytes) into the buffer object to fill.
/// @param[in] size The size (in bytes) of the buffer object to fill.
/// @param[in] pattern_pointer The pointer to the pattern to read and fill the
/// buffer with, this data must be copied by the device.
/// @param[in] pattern_size The size (in bytes) of `pattern_pointer`, the
/// maximum size is 128 bytes which is the size of the largest supported vector
/// type.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandFillBuffer(mux_command_buffer_t command_buffer,
                                  mux_buffer_t buffer, uint64_t offset,
                                  uint64_t size, const void *pattern_pointer,
                                  uint64_t pattern_size,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point);

/// @brief Push a read image command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the image read command
/// to.
/// @param[in] image The image to read from.
/// @param[in] offset The x, y, z, offset in pixels into the image to read from.
/// @param[in] extent The width, height, depth in pixels of the image to read
/// from.
/// @param[in] row_size The row size in bytes of the host addressable pointer
/// data.
/// @param[in] slice_size The slice size in bytes of the host addressable
/// pointer data.
/// @param[out] pointer The host addressable pointer to image data to write
/// into.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandReadImage(mux_command_buffer_t command_buffer,
                                 mux_image_t image, mux_offset_3d_t offset,
                                 mux_extent_3d_t extent, uint64_t row_size,
                                 uint64_t slice_size, void *pointer,
                                 uint32_t num_sync_points_in_wait_list,
                                 const mux_sync_point_t *sync_point_wait_list,
                                 mux_sync_point_t *sync_point);

/// @brief Push a write image command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the image write command
/// to.
/// @param[in] image The image to write to.
/// @param[in] offset The x, y, z, offset in pixel into the image to write to.
/// @param[in] extent The width, height, depth in pixels of the image to write
/// to.
/// @param[in] row_size The row size in bytes of the host addressable pointer
/// data.
/// @param[in] slice_size The slice size in bytes of the host addressable
/// pointer data.
/// @param[in] pointer The host addressable pointer to image data to read from.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandWriteImage(mux_command_buffer_t command_buffer,
                                  mux_image_t image, mux_offset_3d_t offset,
                                  mux_extent_3d_t extent, uint64_t row_size,
                                  uint64_t slice_size, const void *pointer,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point);

/// @brief Push a fill image command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the image fill command
/// to.
/// @param[in] image The image to fill.
/// @param[in] color The color to fill the image with, this data must be copied
/// by the device.
/// @param[in] color_size The size (in bytes) of the color.
/// @param[in] offset The x, y, z, offset in pixels into the image to fill.
/// @param[in] extent The width, height, depth in pixels of the image to fill.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandFillImage(mux_command_buffer_t command_buffer,
                                 mux_image_t image, const void *color,
                                 uint32_t color_size, mux_offset_3d_t offset,
                                 mux_extent_3d_t extent,
                                 uint32_t num_sync_points_in_wait_list,
                                 const mux_sync_point_t *sync_point_wait_list,
                                 mux_sync_point_t *sync_point);

/// @brief Push a copy image command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the image copy command
/// to.
/// @param[in] src_image The source image where data will be copied from.
/// @param[in] dst_image The destination image where data will be copied to.
/// @param[in] src_offset The x, y, z offset in pixels into the source image.
/// @param[in] dst_offset The x, y, z offset in pixels into the destination
/// image.
/// @param[in] extent The width, height, depth in pixels range of the images to
/// be copied.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandCopyImage(mux_command_buffer_t command_buffer,
                                 mux_image_t src_image, mux_image_t dst_image,
                                 mux_offset_3d_t src_offset,
                                 mux_offset_3d_t dst_offset,
                                 mux_extent_3d_t extent,
                                 uint32_t num_sync_points_in_wait_list,
                                 const mux_sync_point_t *sync_point_wait_list,
                                 mux_sync_point_t *sync_point);

/// @brief Push a copy image to buffer command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the image to buffer
/// copy to.
/// @param[in] src_image The source image to copy data from.
/// @param[in] dst_buffer The destination buffer to copy data to.
/// @param[in] src_offset The x, y, z, offset in pixels into the source image to
/// copy.
/// @param[in] dst_offset The offset in bytes into the destination buffer to
/// copy.
/// @param[in] extent The width, height, depth in pixels of the image to copy.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandCopyImageToBuffer(
    mux_command_buffer_t command_buffer, mux_image_t src_image,
    mux_buffer_t dst_buffer, mux_offset_3d_t src_offset, uint64_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a copy buffer to image command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the buffer to image
/// copy command to.
/// @param[in] src_buffer The source buffer to copy data from.
/// @param[in] dst_image The destination image to copy data to.
/// @param[in] src_offset The offset in bytes into the source buffer to copy.
/// @param[in] dst_offset The x, y, z, offset in pixels into the destination
/// image to copy.
/// @param[in] extent The width, height, depth in pixels range of the data to
/// copy.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandCopyBufferToImage(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_image_t dst_image, uint32_t src_offset, mux_offset_3d_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push an N-Dimensional run command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the N-Dimensional run
/// command to.
/// @param[in] kernel The kernel to execute.
/// @param[in] options The execution options to use during the run command.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandNDRange(mux_command_buffer_t command_buffer,
                               mux_kernel_t kernel,
                               mux_ndrange_options_t options,
                               uint32_t num_sync_points_in_wait_list,
                               const mux_sync_point_t *sync_point_wait_list,
                               mux_sync_point_t *sync_point);

/// @brief Update arguments to an N-Dimensional run command within the command
/// buffer.
///
/// @param[in] command_buffer The command buffer containing an N-Dimensional run
/// command to update.
/// @param[in] command_id The unique ID representing the index of the ND range
/// command within its containing command buffer.
/// @param[in] num_args Number of arguments to the kernel to be updated.
/// @param[in] arg_indices Indices of the arguments to be updated in the order
/// they appear as parameters to the kernel.
/// @param[in] descriptors Array of num_args argument descriptors which are the
/// new values for the arguments to the kernel.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxUpdateDescriptors(mux_command_buffer_t command_buffer,
                                  mux_command_id_t command_id,
                                  uint64_t num_args, uint64_t *arg_indices,
                                  mux_descriptor_info_t *descriptors);

/// @brief Push a user callback to the command buffer.
///
/// User callback commands allow a command to be placed within a command buffer
/// that will call back into user code. Care should be taken that the work done
/// within the user callback is minimal, as this will stall any forward progress
/// on the command buffer.
///
/// @param[in] command_buffer The command buffer to push the user callback
/// command to.
/// @param[in] user_function The user callback to invoke, must not be null.
/// @param[in] user_data The data to pass to the user callback on invocation,
/// may be null.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxCommandUserCallback(
    mux_command_buffer_t command_buffer,
    mux_command_user_callback_t user_function, void *user_data,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a begin query command to the command buffer.
///
/// Begin query commands enable a query pool for use storing query results.
///
/// @param[in] command_buffer The command buffer to push the begin query command
/// to.
/// @param[in] query_pool The query pool to store the query result in.
/// @param[in] query_index The initial query slot index that will contain the
/// result.
/// @param[in] query_count The number of query slots to enable.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* error code in an error occurred.
mux_result_t muxCommandBeginQuery(mux_command_buffer_t command_buffer,
                                  mux_query_pool_t query_pool,
                                  uint32_t query_index, uint32_t query_count,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point);

/// @brief Push a begin query command to the command buffer.
///
/// End query commands disable a query pool from use for storing query results.
///
/// @param[in] command_buffer The command buffer to push the end query command
/// to.
/// @param[in] query_pool The query pool the result is stored in.
/// @param[in] query_index The initial query slot index that contains the
/// result.
/// @param[in] query_count The number of query slots to disable.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* error code in an error occurred.
mux_result_t muxCommandEndQuery(mux_command_buffer_t command_buffer,
                                mux_query_pool_t query_pool,
                                uint32_t query_index, uint32_t query_count,
                                uint32_t num_sync_points_in_wait_list,
                                const mux_sync_point_t *sync_point_wait_list,
                                mux_sync_point_t *sync_point);

/// @brief Push a reset query pool command to the command buffer.
///
/// Reset query pool commands enable reuse of the query pool as if it was newly
/// created.
///
/// @param[in] command_buffer The command buffer to push the query pool reset
/// command to.
/// @param[in] query_pool The query pool to reset.
/// @param[in] query_index The first query index to reset.
/// @param[in] query_count The number of query slots to reset.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* error code in an error occurred.
mux_result_t muxCommandResetQueryPool(
    mux_command_buffer_t command_buffer, mux_query_pool_t query_pool,
    uint32_t query_index, uint32_t query_count,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Reset a command buffer.
///
/// @param[in] command_buffer The command buffer to reset.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxResetCommandBuffer(mux_command_buffer_t command_buffer);

/// @brief Push a command buffer to a queue.
///
/// Push a command buffer to a queue.
///
/// @param[in] queue A Mux queue.
/// @param[in] command_buffer A command buffer to push to a queue.
/// @param[in] fence A fence to signal when the dispatch completes.
/// @param[in] wait_semaphores An array of semaphores that this dispatch must
/// wait on before executing.
/// @param[in] wait_semaphores_length The length of wait_semaphores.
/// @param[in] signal_semaphores An array of semaphores that this dispatch will
/// signal when complete.
/// @param[in] signal_semaphores_length The length of signal_semaphores.
/// @param[in] user_function The command buffer complete callback, may be null.
/// @param[in] user_data The data to pass to the command buffer complete
/// callback on completion, may be null.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxDispatch(
    mux_queue_t queue, mux_command_buffer_t command_buffer, mux_fence_t fence,
    mux_semaphore_t *wait_semaphores, uint32_t wait_semaphores_length,
    mux_semaphore_t *signal_semaphores, uint32_t signal_semaphores_length,
    void (*user_function)(mux_command_buffer_t command_buffer,
                          mux_result_t error, void *const user_data),
    void *user_data);

/// @brief Try to wait for a fence to be signaled.
///
/// If the fence is not yet signaled, return mux_fence_not_ready.
///
/// @param[in] queue A Mux queue.
/// @param[in] timeout The timeout period in units of nanoseconds. If the fence
/// waited on isn't signaled before timeout nanoseconds after the API is called,
/// tryWait will return with the fence having not been signaled. timeout ==
/// UINT64_MAX indicates an infinite timeout period and the entry point will
/// block until the fence is signaled.
/// @param[in] fence A fence to wait on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxTryWait(mux_queue_t queue, uint64_t timeout, mux_fence_t fence);

/// @brief Wait for all previously pushed command buffers to complete.
///
/// Wait for all previously pushed command buffers to complete.
///
/// @param[in] queue A Mux queue.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t muxWaitAll(mux_queue_t queue);

/// @brief Mux's allocator container.
///
/// This struct is used to pass in the allocator functions, and the user data
/// required, to a Mux device.
struct mux_allocator_info_s {
  /// @brief Callback to allocate aligned memory.
  void *(*alloc)(void *user_data, size_t size, size_t alignment);
  /// @brief Callback to free previously allocated memory.
  void (*free)(void *user_data, void *pointer);
  /// @brief The user data to be used when calling our callbacks.
  void *user_data;
};

/// @brief Mux's callback container.
///
/// This struct is used to pass in a message callback function, and the user
/// data required, to a Mux device. The callback may be invoked by the
/// implementation to provide more detailed information about API usage.
struct mux_callback_info_s {
  /// @brief Callback to provide a message to the user.
  void (*callback)(void *user_data, const char *message, const void *data,
                   size_t data_size);
  /// @brief The user data to be used when calling the callback.
  void *user_data;
};

/// @brief Mux's device information container.
///
/// Holds details about a partner device, allowing the access to them without
/// initializing that device.
struct mux_device_info_s {
  /// @brief The ID of this device object.
  mux_id_t id;
  /// @brief The buffer capabilities of this Mux device, a bitfield.
  ///
  /// @see mux_allocation_capabilities_e
  uint32_t allocation_capabilities;
  /// @brief The addressing capabilities of this Mux device, a bitfield.
  ///
  /// @see mux_address_capabilities_e
  uint32_t address_capabilities;
  /// @brief The caching capabilities of this Mux device, a bitfield.
  ///
  /// @see mux_cache_capabilities_e
  uint32_t cache_capabilities;
  /// @brief The half floating point capabilities of this Mux device, a
  /// bitfield.
  ///
  /// @see mux_floating_point_capabilities_e
  uint32_t half_capabilities;
  /// @brief The floating point capabilities of this Mux device, a bitfield.
  ///
  /// @see mux_floating_point_capabilities_e
  uint32_t float_capabilities;
  /// @brief The double floating point capabilities of this Mux device, a
  /// bitfield.
  ///
  /// @see mux_floating_point_capabilities_e
  uint32_t double_capabilities;
  /// @brief The integer capabilities of this Mux device, a bitfield.
  ///
  /// @see mux_integer_capabilities_e
  uint32_t integer_capabilities;
  /// @brief The custom buffer capabilities of this Mux device, a bitfield.
  ///
  /// @see mux_custom_buffer_capabilities_e
  uint32_t custom_buffer_capabilities;
  /// @brief The endianness of this Mux device.
  ///
  /// @see mux_endianness_e
  uint32_t endianness;
  /// @brief The unique Khronos-allocated vendor ID of this Mux device.
  ///
  /// This value is assigned by Khronos to all fee-paying implementers of
  /// Khronos standards, used primarily to differentiate between different
  /// vendor implementations of standards. For Mux, we allow our partners to
  /// return 0 for applications that do not want to advertise that they support
  /// any of the Khronos standards, but if a partner does want to be able to
  /// tell their own userbase, or use Khronos conformance to any standard as
  /// part of any marketing materials, they must either have their own allocated
  /// Khronos ID, or use Codeplay's Khronos ID.
  ///
  /// For OpenCL, this value is queryable by calling clGetDeviceInfo, asking for
  /// CL_DEVICE_VENDOR_ID.
  ///
  /// @see MUX_DEVICE_KHRONOS_CODEPLAY_ID
  uint32_t khronos_vendor_id;
  /// @brief The type of this Mux device's shared local memory.
  ///
  /// This value specifies which type a Mux device's shared local memory is.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_LOCAL_MEM_TYPE.
  ///
  /// @see mux_shared_local_memory_type_e
  uint32_t shared_local_memory_type;
  /// @brief The type of this Mux device.
  ///
  /// This value specifies which type a Mux device is. This is used to provide
  /// information to users of the various standards that sit above Mux as to
  /// what each device actually is.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_TYPE.
  ///
  /// @see mux_device_type_e
  uint32_t device_type;
  /// @brief A semicolon-separated, null-terminated list of built-in kernel
  /// declarations.
  ///
  /// The declarations are of the form `kernel_name(parameters)`, e.g.,
  /// `a_kernel(global float* foo, int bar)`. Kernel names and parameters
  /// conform to the OpenCL specification for kernel names and kernel parameters
  /// with the following exceptions:
  ///
  /// 1.  The leading `__kernel` keyword **must not** be used
  /// 2.  The return type (`void`) **must** be omitted
  /// 3.  Pointer parameters using `[]` notation **must not** be used
  /// 4.  Struct parameters and pointer-to-struct parameters **must not** be
  /// used
  /// 5.  Type and variable attributes (e.g., `__attribute__*`) **must not** be
  /// used
  /// 6.  All whitespace **must** only be ` ` characters; other whitespace
  /// characters (`\t`, `\n`, etc.) **must not** be used
  /// 7.  The built-in kernel name including a trailing NUL character must not
  /// exceed
  ///     `CL_NAME_VERSION_KHR` (64 characters).
  ///
  /// Also note that:
  ///
  /// 1.  Pointer-to-pointer parameters are not supported.
  /// 2.  The `const` and `volatile` type qualifiers are ignored when used with
  /// a value parameter. I.e., `const int foo` and `int foo` are equivalent.
  const char *builtin_kernel_declarations;
  /// @brief A null-terminated string, with static lifetime duration, of this
  /// Mux device's name.
  ///
  /// This value is a human-readable variant of the device_type field. It is
  /// used for standards that return a string to users to help them identify
  /// whose implementation of a given standard they are using.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_NAME.
  const char *device_name;
  /// @brief The maximum number of work items in a work group that can run
  /// concurrently.
  ///
  /// The maximum number of work items that can inhabit a work/thread group.
  /// Various standards refer to work items and work/thread groups using
  /// different lingo, but at the heart work items should execute on a given
  /// device concurrently, up to this maximum number of work items.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_MAX_WORK_GROUP_SIZE.
  uint32_t max_concurrent_work_items;
  /// @brief The maximum work group size in the x dimension.
  ///
  /// The maximum number of work items allowed in the x dimension of a work
  /// group.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_MAX_WORK_ITEM_SIZES.
  uint32_t max_work_group_size_x;
  /// @brief The maximum work group size in the y dimension.
  ///
  /// The maximum number of work items allowed in the y dimension of a work
  /// group.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_MAX_WORK_ITEM_SIZES.
  uint32_t max_work_group_size_y;
  /// @brief The maximum work group size in the z dimension.
  ///
  /// The maximum number of work items allowed in the z dimension of a work
  /// group.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_MAX_WORK_ITEM_SIZES.
  uint32_t max_work_group_size_z;
  /// @brief The maximum work width.
  ///
  /// The maximum number of work items of a work group allowed to execute in one
  /// invocation of a kernel.
  uint32_t max_work_width;
  /// @brief The clock frequency (in MHz) of this Mux device.
  ///
  /// Used for standards that want to know the frequency of the chip they are
  /// going to execute with.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_MAX_CLOCK_FREQUENCY.
  uint32_t clock_frequency;
  /// @brief The number of compute units in this Mux device.
  ///
  /// Used for standards that want to know how many distinct compute units make
  /// up a given Mux device.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_MAX_COMPUTE_UNITS.
  uint32_t compute_units;
  /// @brief The alignment (in bytes) of buffer's allocated via this device.
  ///
  /// Used for standards that need to know what alignment buffer objects will
  /// have in device memory. Must be a power of 2.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE.
  uint32_t buffer_alignment;
  /// @brief The size (in bytes) of our device memory.
  ///
  /// Used for standards that want to know the total size of memory allocatable
  /// from each device.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_GLOBAL_MEM_SIZE.
  uint64_t memory_size;
  /// @brief The maximum size (in bytes) of a single device memory allocation.
  ///
  /// Used for standards that want to know the maximum size of a single memory
  /// allocation from each device. For some devices this value may be the same
  /// as memory_size, i.e. there is no single allocation limit beyond the total
  /// memory capacity.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_MAX_MEM_ALLOC_SIZE. For OpenCL full profile a minimum value
  /// of 128MiB (128*1024*1024) will be required, for embedded profile a minimum
  /// value of 1MiB (1*1024*1024) will be required.
  ///
  /// @note Although OpenCL also generally calculates maximum allocation size as
  /// 1/4 of total memory size, this is *not* required here, it is the OpenCL
  /// implementation's responsibility to consider such things.
  uint64_t allocation_size;
  /// @brief The size (in bytes) of our device memory cache.
  ///
  /// Used for standards that want to know the total size of memory that is
  /// cached by the device.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_GLOBAL_MEM_CACHE_SIZE.
  ///
  uint64_t cache_size;
  /// @brief The size (in bytes) of our device memory cache line.
  ///
  /// Used for standards that want to know the total size of memory that is
  /// held, in each line of a cache, by the device.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE.
  ///
  uint64_t cacheline_size;
  /// @brief The size (in bytes) of our shared local device memory.
  ///
  /// Used for standards that want to know the total size of memory allocatable
  /// in shared local memory from each device.
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for CL_DEVICE_LOCAL_MEM_SIZE.
  ///
  uint64_t shared_local_memory_size;
  /// @brief Native vector width of the device (in bytes).
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for all of the CL_DEVICE_NATIVE_VECTOR_WIDTH_* queries.
  uint32_t native_vector_width;
  /// @brief Preferred vector width of the device (in bytes).
  ///
  /// For OpenCL, this value is used to implement clGetDeviceInfo, when asking
  /// for all of the CL_DEVICE_PREFERRED_VECTOR_WIDTH_* queries.
  uint32_t preferred_vector_width;
  /// @brief Is @p true if the device supports images, @p false otherwise.
  bool image_support;
  /// @brief Is @p true if the device supports 2D image array writes, @p false
  /// otherwise.
  bool image2d_array_writes;
  /// @brief Is @p true if the device supports 3D image writes, @p false
  /// otherwise.
  bool image3d_writes;
  /// @brief The maximum dimension size in pixels for a one dimensional image,
  /// zero if images are not supported.
  uint32_t max_image_dimension_1d;
  /// @brief The maximum dimension size in pixels for a two dimensional image,
  /// zero if images are not supported.
  uint32_t max_image_dimension_2d;
  /// @brief The maximum dimension size in pixels for a three dimensional image,
  /// zero if images are not supported.
  uint32_t max_image_dimension_3d;
  /// @brief The maximum array layers count for an image array, zero if images
  /// are not supported.
  uint32_t max_image_array_layers;
  /// @brief The maximum number of bound images for image storage (write), zero
  /// if images are not supported.
  uint32_t max_storage_images;
  /// @brief The maximum number of bound images for image sampling (read), zero
  /// if images are not supported.
  uint32_t max_sampled_images;
  /// @brief The maximum number of bound samplers, zero if images are not
  /// supported.
  uint32_t max_samplers;
  /// @brief The number of queues of each type that are available with this
  /// device.
  uint32_t queue_types[mux_queue_type_total];
  /// @brief The priority of devices on the system for deciding which should be
  /// default device.
  ///
  /// This value is used for tracking device priority when deciding which
  /// devices shall be returned if `CL_DEVICE_TYPE_DEFAULT` is requested. Host
  /// will have a value of `0` where higher priority devices will be in the
  /// positive range and conversely a lower priority for the negative range.
  int8_t device_priority;
  /// @brief If `true` the device supports `mux_query_type_counter`, `false`
  /// otherwise.
  bool query_counter_support;
  /// @brief If `true` the device supports updating the `mux_device_info_t`
  /// argument descriptors passed to a specialized kernel in a
  /// `muxCommandNDRange` command after the containing `mux_command_buffer` has
  /// been finalized.
  bool descriptors_updatable;
  /// @brief If `true` the device supports cloning `mux_command_buffers` via the
  /// `muxCloneCommandBuffer` entry point.
  bool can_clone_command_buffers;
  /// @brief If `true` the device supports creating built-in kernels via the
  /// `muxCreateBuiltInKernel` entry point.
  bool supports_builtin_kernels;
  /// @brief The maximum number of sub-groups in a work-group. A target not
  /// supporting sub-groups must set this to `0`.
  uint32_t max_sub_group_count;
  /// @brief If `true` the device supports independent forward progress in its
  /// sub-groups. A target not supporting sub-groups must set this to `false`.
  bool sub_groups_support_ifp;
  /// @brief Maximum number of hardware counters that can be active at one time.
  uint32_t max_hardware_counters;
  /// @brief Boolean value indicating if work-group collective functions are
  /// supported by the device.
  bool supports_work_group_collectives;
  /// @brief Boolean value indicating if the generic address space is supported
  /// by the device.
  bool supports_generic_address_space;
  /// @brief The number of sub-group sizes supported by the device, pointed to
  /// by sub_group_sizes.
  size_t num_sub_group_sizes;
  /// @brief List of sub-group sizes supported by the device, sized by
  /// num_sub_group_sizes.
  size_t *sub_group_sizes;
};

/// @brief Mux's device container.
///
/// The entry struct into all partner code - the device. Each partner must
/// register at least one device (by defining hooks of type coreGetDeviceInfos_t
/// and coreCreateDevices_t). All other partner code is interfaced with through
/// instances of this device struct.
struct mux_device_s {
  /// @brief The ID of this device object, matches the device info ID.
  mux_id_t id;
  /// @brief The information associated with this device.
  mux_device_info_t info;
};

/// @brief Description of the device memory requirements of a buffer or an
/// image.
typedef struct mux_memory_requirements_s {
  /// @brief The size in bytes of device memory required.
  uint64_t size;
  /// @brief The alignment in bytes of the required device memory offset.
  uint32_t alignment;
  /// @brief Bitfield of device memory heaps which support this buffer or image.
  /// The bitfield must have at least one bit set, for implementations with only
  /// one heap the first bit must be set.
  uint32_t supported_heaps;
} mux_memory_requirements_s;

/// @brief Mux's memory container.
///
/// Each Mux device can allocate memory which can be bound to objects requiring
/// device memory, such as buffers and images.
struct mux_memory_s {
  /// @brief The ID of this memory object.
  mux_id_t id;
  /// @brief The size in bytes of the allocated device memory.
  uint64_t size;
  /// @brief The memory properties of the allocated device memory.
  uint32_t properties;
  /// @brief A handle to the allocated memory. For memory objects with the
  /// mux_memory_property_host_visible property this is a host accessible
  /// pointer.
  uint64_t handle;
};

/// @brief Mux's buffer container.
///
/// Each Mux device can allocate buffers for use when executing programs on a
/// device.
struct mux_buffer_s {
  /// @brief The ID of this buffer object.
  mux_id_t id;
  /// @brief The device memory requirements for this buffer.
  mux_memory_requirements_s memory_requirements;
};

/// @brief Describes an offset in three dimensions.
struct mux_offset_3d_s {
  /// @brief The value of the x dimension of the three dimensional offset.
  uint32_t x;
  /// @brief The value of the y dimension of the three dimensional offset.
  uint32_t y;
  /// @brief The value of the z dimension of the three dimensional offset.
  uint32_t z;
};

/// @brief Describes a range in three dimensions.
struct mux_extent_3d_s {
  /// @brief The value of the x dimension of the three dimensional extent.
  size_t x;
  /// @brief The value of the y dimension of the three dimensional extent.
  size_t y;
  /// @brief The value of the z dimension of the three dimensional extent.
  size_t z;
};

/// @brief Describes a range in two dimensions.
struct mux_extent_2d_s {
  /// @brief The value of the x dimension of the two dimensional extent.
  size_t x;
  /// @brief The value of the y dimension of the two dimensional extent.
  size_t y;
};

/// @brief Mux's image container.
///
/// Each Mux device can allocate images for use when executing programs on a
/// device.
struct mux_image_s {
  /// @brief The ID of this image object.
  mux_id_t id;
  /// @brief The device memory requirements for this image.
  mux_memory_requirements_s memory_requirements;
  /// @brief The type of this image.
  mux_image_type_e type;
  /// @brief The pixel format of this image.
  mux_image_format_e format;
  /// @brief The size in bytes of a single pixel.
  uint32_t pixel_size;
  /// @brief The width, height, depth of this image.
  mux_extent_3d_t size;
  /// @brief The number of array layers in the image.
  uint32_t array_layers;
  /// @brief The size in bytes of an image row.
  uint64_t row_size;
  /// @brief The size in bytes of an image slice.
  uint64_t slice_size;
  /// @brief The current image tiling mode of this image.
  mux_image_tiling_e tiling;
};

/// @brief A container describing a sampler.
struct mux_sampler_s {
  /// @brief The ID of this sampler object.
  mux_id_t id;
  /// @brief Specify what happens when an out-of-range image coordinate is
  /// specified.
  mux_address_mode_e address_mode;
  /// @brief Specify what kind of filter to user when reading image data.
  mux_filter_mode_e filter_mode;
  /// @brief If true sampler expects coordinates to be normalized, if false
  /// sampler does not expect normalized coordinates.
  bool normalize_coords;
};

/// @brief Mux's queue container.
///
/// Each Mux device has zero or more queues to execute programs, run commands,
/// and interact with the asynchronous device. This struct contains members that
/// denote information about this queue.
struct mux_queue_s {
  /// @brief The ID of this queue object.
  mux_id_t id;
  /// @brief The Mux device that this object was got from.
  mux_device_t device;
};

/// @brief Mux's semaphore container.
///
/// Each Mux device can allocate a semaphore that can be used to synchronize on
/// device between command buffer's.
struct mux_semaphore_s {
  /// @brief The ID of this semaphore object.
  mux_id_t id;
  /// @brief The Mux device that this object was created from.
  mux_device_t device;
};

/// @brief Mux's fence container.
///
/// Each Mux device can allocate a fence that can be used to synchronize from
/// device to host.
struct mux_fence_s {
  /// @brief The ID of this fence object.
  mux_id_t id;
  /// @brief The Mux device that this object was created from.
  mux_device_t device;
};

/// @brief Mux's sync-point container.
///
/// Each Mux command-buffer command can return a sync-point object that can be
/// used to synchronize on commands inside a command-buffer.
struct mux_sync_point_s {
  /// @brief The ID of this sync-point object.
  mux_id_t id;
  /// @brief The Mux command-buffer that this object was created from.
  mux_command_buffer_t command_buffer;
};

/// @brief Mux's executable container.
struct mux_executable_s {
  /// @brief The ID of this executable object.
  mux_id_t id;
  /// @brief The Mux device that this object was created from.
  mux_device_t device;
};

/// @brief Mux's kernel container.
struct mux_kernel_s {
  /// @brief The ID of this kernel object.
  mux_id_t id;
  /// @brief The Mux device that this object was created from.
  mux_device_t device;
  /// @brief The preferred local size in the x dimension for this kernel.
  size_t preferred_local_size_x;
  /// @brief The preferred local size in the y dimension for this kernel.
  size_t preferred_local_size_y;
  /// @brief The preferred local size in the z dimension for this kernel.
  size_t preferred_local_size_z;
  /// @brief The amount of local memory used by this kernel.
  size_t local_memory_size;
};

/// @brief Describes a kernel's execution options.
struct mux_ndrange_options_s {
  /// @brief Array of argument descriptors.
  mux_descriptor_info_t *descriptors;
  /// @brief Length of the descriptors array.
  uint64_t descriptors_length;
  /// @brief Local size in the x, y and z dimensions.
  size_t local_size[3];
  /// @brief Array of global offsets (can be null).
  const size_t *global_offset;
  /// @brief Array of global sizes.
  const size_t *global_size;
  /// @brief The length of `global_offset` and `global_size` (if they are
  /// non-null).
  size_t dimensions;
};

/// @brief Mux's command buffer container.
struct mux_command_buffer_s {
  /// @brief The ID of this command buffer object.
  mux_id_t id;
  /// @brief The Mux device that this object was created from.
  mux_device_t device;
};

/// @brief Mux's query pool container.
struct mux_query_pool_s {
  /// @brief The ID of this query pool object.
  mux_id_t id;
  /// @brief The type of the query pool (one of the values in the enum
  /// mux_query_type_e).
  mux_query_type_e type;
  /// @brief The total number of queries that can be stored in the query pool.
  uint32_t count;
};

/// @brief Mux's query counter information container.
struct mux_query_counter_s {
  /// @brief The unit of the query counter result.
  mux_query_counter_unit_e unit;
  /// @brief The storage type of the query counter result.
  mux_query_counter_storage_e storage;
  /// @brief The unique ID of the query counter.
  uint32_t uuid;
  /// @brief How many hardware counters this query counter uses to take its
  /// measurement.
  uint32_t hardware_counters;
};

/// @brief Mux's query counter description container.
struct mux_query_counter_description_s {
  /// @brief The counter name stored in a UTF-8 encoded null terminated array of
  /// 256 characters.
  char name[256];
  /// @brief The counter category stored in a UTF-8 encoded null terminated
  /// array of 256 characters.
  char category[256];
  /// @brief The counter description stored in a UTF-8 encoded null terminated
  /// array of 256 characters.
  char description[256];
};

/// @brief Mux's query counter configuration container.
struct mux_query_counter_config_s {
  /// @brief The unique ID of the query counter.
  uint32_t uuid;
  /// @brief Data used to specify how a counter is to be configured, **may** be
  /// NULL.
  void *data;
};

/// @brief Mux's duration query result container.
struct mux_query_duration_result_s {
  /// @brief The CPU timestamp at the start of the command, in nanoseconds.
  uint64_t start;
  /// @brief The CPU timestamp at the end of the command, in nanoseconds.
  uint64_t end;
};

/// @brief Mux's counter query result container.
struct mux_query_counter_result_s {
  /// @brief The union of possible counter query results.
  union {
    int32_t int32;
    int64_t int64;
    uint32_t uint32;
    uint64_t uint64;
    float float32;
    double float64;
  };
};

/// @brief A Mux descriptor for buffers.
struct mux_descriptor_info_buffer_s {
  /// @brief Buffer descriptor.
  mux_buffer_t buffer;
  /// @brief Offset in bytes into @p buffer.
  uint64_t offset;
};

/// @brief A Mux descriptor for images.
struct mux_descriptor_info_image_s {
  /// @brief Image object.
  mux_image_t image;
};

/// @brief A Mux descriptor for samplers.
struct mux_descriptor_info_sampler_s {
  /// @brief Sampler object.
  mux_sampler_t sampler;
};

/// @brief A Mux descriptor for plain old data.
struct mux_descriptor_info_plain_old_data_s {
  /// @brief Pointer to plain old data.
  const void *data;
  /// @brief Size in bytes of @p data storage.
  size_t length;
};

/// @brief A Mux descriptor for a shared local buffer.
struct mux_descriptor_info_shared_local_buffer_s {
  /// @brief Size in bytes of shared local buffer.
  size_t size;
};

/// @brief A Mux descriptor for a custom buffer.
struct mux_descriptor_info_custom_buffer_s {
  /// @brief Pointer to custom data representing the buffer.
  const void *data;
  /// @brief Size in bytes of the memory pointed to by `data`.
  size_t size;
  /// @brief Address space of the custom buffer.
  uint32_t address_space;
};

/// @brief A Mux descriptor for passing null instead of a pointer to buffer data
/// to a kernel.
struct mux_descriptor_info_null_buffer_s;

/// @brief Mux's descriptor container.
struct mux_descriptor_info_s {
  /// @brief The type of this descriptor (one of the values in the enum
  /// mux_descriptor_info_type_e).
  uint32_t type;
  union {
    struct mux_descriptor_info_buffer_s buffer_descriptor;
    struct mux_descriptor_info_image_s image_descriptor;
    struct mux_descriptor_info_sampler_s sampler_descriptor;
    struct mux_descriptor_info_plain_old_data_s plain_old_data_descriptor;
    struct mux_descriptor_info_shared_local_buffer_s
        shared_local_buffer_descriptor;
    struct mux_descriptor_info_custom_buffer_s custom_buffer_descriptor;
  };
};

/// @brief A buffer region.
struct mux_buffer_region_info_s {
  /// @brief The dimensions of the region of memory. If a 2D shape z must be 1,
  /// if a 1D shape y and z must be 1.
  mux_extent_3d_t region;
  /// @brief In bytes, source origin offset expressed in 3D.
  mux_extent_3d_t src_origin;
  /// @brief In bytes, destination origin offset, expressed in 3D.
  mux_extent_3d_t dst_origin;
  /// @brief The description of the source buffer, where x represents the length
  /// of a row, and y represents the size of a 2D slice. If a 1D shape y must
  /// be 1.
  mux_extent_2d_t src_desc;
  /// @brief The description of the destination buffer, where x represents the
  /// length of a row, and y represents the size of a 2D slice. If a 1D shape y
  /// must be 1.
  mux_extent_2d_t dst_desc;
};

/// @}

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // MUX_MUX_H_INCLUDED
