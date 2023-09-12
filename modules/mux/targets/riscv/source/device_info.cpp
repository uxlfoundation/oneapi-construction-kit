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

#include <mux/mux.h>
#include <riscv/device_info.h>
#include <riscv/device_info_get.h>
#include <riscv/hal.h>
#include <riscv/riscv.h>

#include <array>
#include <cassert>
#include <limits>

namespace riscv {

void device_info_s::update_from_hal_info(const hal::hal_device_info_t *info) {
  hal_device_info = info;
  this->shared_local_memory_size = info->shared_local_memory_size;
  this->address_capabilities = info->word_size == 32
                                   ? mux_address_capabilities_bits32
                                   : mux_address_capabilities_bits64;

  this->half_capabilities = info->supports_fp16
                                ? (mux_floating_point_capabilities_inf_nan |
                                   mux_floating_point_capabilities_rte |
                                   mux_floating_point_capabilities_full)
                                : 0;
  this->double_capabilities = info->supports_doubles
                                  ? (mux_floating_point_capabilities_denorm |
                                     mux_floating_point_capabilities_inf_nan |
                                     mux_floating_point_capabilities_rte |
                                     mux_floating_point_capabilities_rtz |
                                     mux_floating_point_capabilities_rtp |
                                     mux_floating_point_capabilities_rtn |
                                     mux_floating_point_capabilities_fma)
                                  : 0;
  this->device_name = info->target_name;
  this->memory_size = info->global_memory_avail;
  this->allocation_size = info->global_memory_avail;
  this->native_vector_width = info->preferred_vector_width;
  this->preferred_vector_width = info->preferred_vector_width;
  this->endianness =
      info->is_little_endian ? mux_endianness_little : mux_endianness_big;
  this->max_concurrent_work_items = info->max_workgroup_size;
  this->max_work_group_size_x = this->max_concurrent_work_items;
  this->max_work_group_size_y = this->max_concurrent_work_items;
  this->max_work_group_size_z = this->max_concurrent_work_items;
  this->query_counter_support = (info->num_counters > 0);

  // device info has been updated from the hal and is now valid
  valid = true;
}

device_info_s::device_info_s()
    : hal_device_info(nullptr), hal_device_index(0), valid(false) {
  this->allocation_capabilities = mux_allocation_capabilities_alloc_device;

  // Override this default from hal updates
  this->address_capabilities = mux_address_capabilities_bits64;

  this->cache_capabilities =
      mux_cache_capabilities_read | mux_cache_capabilities_write;

  this->half_capabilities = 0;

  this->float_capabilities = mux_floating_point_capabilities_denorm |
                             mux_floating_point_capabilities_inf_nan |
                             mux_floating_point_capabilities_rte |
                             mux_floating_point_capabilities_full;

  this->double_capabilities = 0;

  // As an ISV without a unique device PCIe identifier, 0x10004 is the vendor ID
  // we've reserved in Khronos specs. Matches enums
  // 'CL_KHRONOS_VENDOR_ID_CODEPLAY' from OpenCL and 'VK_VENDOR_ID_CODEPLAY'
  // from Vulkan, but we can't use these symbols here because it would
  // introduce unwanted dependencies to our mux target.
  this->khronos_vendor_id = 0x10004;
  this->shared_local_memory_type = mux_shared_local_memory_physical;
  this->device_type = mux_device_type_accelerator;
  this->device_name = "riscv";
  this->max_concurrent_work_items = 1024;
  this->max_work_group_size_x = this->max_concurrent_work_items;
  this->max_work_group_size_y = this->max_concurrent_work_items;
  this->max_work_group_size_z = this->max_concurrent_work_items;
  this->max_work_width = max_concurrent_work_items;
  this->clock_frequency = 1;  // arbitrary default non-zero clock frequency to
                              // satisfy OpenCL requirements
  this->compute_units = 1;
  this->buffer_alignment = sizeof(uint64_t) * 16;
  this->memory_size =
      10 * 1024 * 1024;  // default value - should be updated using hal values
  // All memory could be allocated at once.
  this->allocation_size = this->memory_size;
  this->cache_size = 0;
  this->cacheline_size = 0;
  this->shared_local_memory_size = 32 * 1024;

  // default to 128 bit (16 bytes)
  this->native_vector_width = 128 / (8 * sizeof(uint8_t));
  this->preferred_vector_width = 128 / (8 * sizeof(uint8_t));

  this->image_support = false;
  this->image2d_array_writes = false;
  this->image3d_writes = false;
  this->max_image_dimension_1d = 0;
  this->max_image_dimension_2d = 0;
  this->max_image_dimension_3d = 0;
  this->max_image_array_layers = 0;
  this->max_sampled_images = 0;
  this->max_storage_images = 0;
  this->max_samplers = 0;

  // we have only one queue on riscv
  this->queue_types[mux_queue_type_compute] = 1;

  this->device_priority = 0;

  this->integer_capabilities =
      mux_integer_capabilities_8bit | mux_integer_capabilities_16bit |
      mux_integer_capabilities_32bit | mux_integer_capabilities_64bit;

  this->endianness = mux_endianness_little;
  this->builtin_kernel_declarations = "";
  this->query_counter_support = true;
  this->descriptors_updatable = true;
  this->supports_builtin_kernels = false;
  this->can_clone_command_buffers = true;
  this->max_sub_group_count = this->max_concurrent_work_items;
  this->sub_groups_support_ifp = false;
  // No known upper limit, so just make it something big enough to not matter.
  this->max_hardware_counters = std::numeric_limits<int>::max();
  this->supports_work_group_collectives = true;
  this->supports_generic_address_space = true;

  static std::array<size_t, 1> sg_sizes = {
      1,  // we can always produce a 'trivial' sub-group if asked.
  };
  this->sub_group_sizes = sg_sizes.data();
  this->num_sub_group_sizes = sg_sizes.size();
}

mux_result_t GetDeviceInfos(uint32_t device_types, uint64_t device_infos_length,
                            mux_device_info_t *out_device_infos,
                            uint64_t *out_device_infos_length) {
  // ensure our device infos have been enumerated
  if (!riscv::enumerate_device_infos()) {
    return mux_error_failure;
  }
  // iterate over the available device infos
  uint32_t num_infos_out = 0;
  for (auto &info : GetDeviceInfosArray()) {
    // list ends with first non valid info entry
    if (!info.is_valid()) {
      break;
    }
    // skip device types that were not requested
    if (0 == (info.device_type & device_types)) {
      continue;
    }

    // check processor word size compatibility
    const bool HAL_32_bit =
        info.address_capabilities & mux_address_capabilities_bits32;
    if (bool(CA_RISCV_32_BIT) ^ HAL_32_bit) {
      // Mux and HAL bit width mismatch
      continue;
    }
    // check for double support compatibility
    const bool HAL_supports_double = info.double_capabilities;
    if (bool(CA_RISCV_FP_64) ^ HAL_supports_double) {
      // Mux and HAL 64-bit floating point mismatch
      continue;
    }

    // copy out the device info pointer
    if (out_device_infos) {
      out_device_infos[num_infos_out] = static_cast<mux_device_info_t>(&info);
    }
    // advance to next device info
    ++num_infos_out;
    if (num_infos_out >= device_infos_length) {
      // no more space so terminate
      break;
    }
  }
  // return the number of infos that we have
  if (out_device_infos_length) {
    *out_device_infos_length = num_infos_out;
  }
  // success
  return mux_success;
}

}  // namespace riscv

mux_result_t riscvGetDeviceInfos(uint32_t device_types,
                                 uint64_t device_infos_length,
                                 mux_device_info_t *out_device_infos,
                                 uint64_t *out_device_infos_length) {
  if (0 == device_types) {
    return mux_error_invalid_value;
  }

  if ((nullptr == out_device_infos) && (nullptr == out_device_infos_length)) {
    return mux_error_null_out_parameter;
  }

  if ((0 < device_infos_length) && (nullptr == out_device_infos)) {
    return mux_error_null_out_parameter;
  }

  // If we're only returning a device infos length, we can just call the target
  // hook directly.
  if ((nullptr == out_device_infos) || (device_infos_length == 0)) {
    return riscv::GetDeviceInfos(device_types, 0, nullptr,
                                 out_device_infos_length);
  }

  auto mux_result =
      riscv::GetDeviceInfos(device_types, device_infos_length, out_device_infos,
                            out_device_infos_length);
  if (mux_success != mux_result) {
    return mux_result;
  }

  return mux_success;
}
