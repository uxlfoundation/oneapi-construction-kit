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

#include "riscv/kernel.h"

#include <limits>

#include "hal_riscv.h"
#include "riscv/device.h"
#include "riscv/executable.h"

namespace riscv {
kernel_s::kernel_s(
    mux::hal::device *device, cargo::string_view name,
    cargo::array_view<uint8_t> object_code, mux::allocator allocator,
    cargo::small_vector<mux::hal::kernel_variant_s, 4> &&variant_data)
    : mux::hal::kernel<mux::hal::kernel_variant_s>(
          device, name, object_code, allocator, std::move(variant_data)) {}

cargo::expected<riscv::kernel_s *, mux_result_t> kernel_s::create(
    riscv::device_s *device, riscv::executable_s *executable,
    cargo::string_view name, mux::allocator allocator) {
  auto *hal_device_info = static_cast<const riscv::hal_device_info_riscv_t *>(
      device->hal_device->get_info());

  const unsigned real_vscale = hal_device_info->vlen / 64;
  auto assert_scalable_supported = [hal_device_info]() {
    assert(
        hal_device_info->vlen &&
        "vlen must be known at runtime to calculate scalable subgroup width");
  };

  cargo::small_vector<mux::hal::kernel_variant_s, 4> variants;
  for (auto &meta : executable->kernel_info) {
    if (name != meta.source_name) {
      continue;
    }
    mux::hal::kernel_variant_s variant;
    variant.variant_name = meta.kernel_name;
    variant.sub_group_size = meta.sub_group_size.getKnownMinValue();
    if (meta.sub_group_size.isScalable()) {
      assert_scalable_supported();
      variant.sub_group_size *= real_vscale;
    }
    variant.min_work_width = meta.min_work_item_factor.getKnownMinValue();
    if (meta.min_work_item_factor.isScalable()) {
      assert_scalable_supported();
      variant.min_work_width *= real_vscale;
    }
    variant.pref_work_width = meta.pref_work_item_factor.getKnownMinValue();
    if (meta.pref_work_item_factor.isScalable()) {
      assert_scalable_supported();
      variant.pref_work_width *= real_vscale;
    }
    if (cargo::success != variants.push_back(std::move(variant))) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
  }

  if (variants.empty()) {
    return cargo::make_unexpected(mux_error_missing_kernel);
  }

  auto kernel =
      mux::hal::kernel<mux::hal::kernel_variant_s>::create<riscv::kernel_s>(
          device, executable, name, std::move(variants), allocator);

  if (!kernel) {
    return cargo::make_unexpected(mux_error_out_of_memory);
  }

  kernel.value()->local_memory_size = 0;
  // These preferred local sizes are fairly arbitrary, at the moment the key
  // point is that they are greater than 1 to ensure that the vectorizer,
  // barrier code, and local work items scheduling are used. We work best with
  // powers of two.
  kernel.value()->preferred_local_size_x =
      std::min(64u, device->info->max_work_group_size_x);
  kernel.value()->preferred_local_size_y = 1;
  kernel.value()->preferred_local_size_z = 1;
  return kernel.value();
}

mux_result_t kernel_s::getSubGroupSizeForLocalSize(size_t local_size_x,
                                                   size_t local_size_y,
                                                   size_t local_size_z,
                                                   size_t *out_sub_group_size) {
  mux::hal::kernel_variant_s variant;
  auto err = getKernelVariantForWGSize(local_size_x, local_size_y, local_size_z,
                                       &variant);
  if (err != mux_success) {
    return err;
  }

  // If we've compiled with degenerate sub-groups, the sub-group size is the
  // work-group size.
  if (variant.sub_group_size == 0) {
    *out_sub_group_size = local_size_x * local_size_y * local_size_z;
  } else {
    // Otherwise, on risc-v we always use vectorize in the x-dimension, so
    // sub-groups "go" in the x-dimension.
    *out_sub_group_size =
        std::min(local_size_x, static_cast<size_t>(variant.sub_group_size));
  }
  return mux_success;
}

mux_result_t kernel_s::getLocalSizeForSubGroupCount(size_t sub_group_count,
                                                    size_t *out_local_size_x,
                                                    size_t *out_local_size_y,
                                                    size_t *out_local_size_z) {
  // Grab the maximum sub-group size we've compiled for.
  uint32_t max_sub_group_size = 1;
  for (auto &v : variant_data) {
    max_sub_group_size = std::max(max_sub_group_size, v.sub_group_size);
  }

  // For simplicity, if we're being asked for just the one sub-group, or the
  // kernel's sub-group size is 1, we know we can satisfy the query with a
  // work-group of 1,1,1.
  if (sub_group_count == 1 || max_sub_group_size == 1) {
    *out_local_size_x = 1;
    *out_local_size_y = 1;
    *out_local_size_z = 1;
    return mux_success;
  }

  // For any other sub-group count, we should ensure that the work-group size
  // we report comes back through getKernelVariantForWGSize when it comes to
  // run it.
  *out_local_size_x = sub_group_count * max_sub_group_size;
  *out_local_size_y = 1;
  *out_local_size_z = 1;

  // If the required local work-group size would be an invalid work-group size,
  // return 0,0,0 as per the specification.
  if (*out_local_size_x > device->info->max_work_group_size_x) {
    *out_local_size_x = 0;
    *out_local_size_y = 0;
    *out_local_size_z = 0;
    return mux_success;
  }

#ifndef NDEBUG
  // Double-check that if we were to be asked for the kernel variant for this
  // work-group size we've reported, we'd receive a kernel variant with the
  // same sub-group size as we've assumed for the calculations.
  mux::hal::kernel_variant_s variant;
  const mux_result_t res = getKernelVariantForWGSize(
      *out_local_size_x, *out_local_size_y, *out_local_size_z, &variant);
  if (res != mux_success || variant.sub_group_size != max_sub_group_size) {
    return mux_error_internal;
  }
#endif

  return mux_success;
}

static bool isLegalKernelVariant(const mux::hal::kernel_variant_s &variant,
                                 size_t local_size_x, size_t local_size_y,
                                 size_t local_size_z) {
  (void)local_size_y;
  (void)local_size_z;
  // If the local size isn't a multiple of the minimum work width, we must
  // disregard this kernel.
  if (local_size_x % variant.min_work_width != 0) {
    return false;
  }

  // Degenerate sub-groups are always legal.
  if (variant.sub_group_size != 0) {
    // Else, ensure it cleanly divides the work-group size.
    // FIXME: We could allow more cases here, such as if Y=Z=1 and the last
    // sub-group was equal to the remainder. See CA-4783.
    if (local_size_x % variant.sub_group_size != 0) {
      return false;
    }
  }
  return true;
}

mux_result_t kernel_s::getKernelVariantForWGSize(
    size_t local_size_x, size_t local_size_y, size_t local_size_z,
    mux::hal::kernel_variant_s *out_variant_data) {
  mux::hal::kernel_variant_s *best_variant = nullptr;
  for (auto &v : variant_data) {
    // If the local size isn't a multiple of the minimum work width, we must
    // disregard this kernel.
    if (!isLegalKernelVariant(v, local_size_x, local_size_y, local_size_z)) {
      continue;
    }

    if (!best_variant) {
      // If we've no best variant, this will have to do
      best_variant = &v;
      continue;
    }

    if (v.pref_work_width == best_variant->pref_work_width) {
      // If two variants have the same preferred work width, choose the one
      // that doesn't use degenerate subgroups, if available.
      if (best_variant->sub_group_size == 0 && v.sub_group_size != 0) {
        best_variant = &v;
      }
    } else if (v.pref_work_width > best_variant->pref_work_width &&
               local_size_x >= v.pref_work_width &&
               (local_size_x % v.pref_work_width == 0 ||
                local_size_x % best_variant->pref_work_width != 0)) {
      // Choose the new variant if it executes more work-items optimally and
      // either:
      // * the new variant's preferred width is a good fit, or
      // * the current variant's preferred width isn't a good fit.
      best_variant = &v;
    }
  }
  if (!best_variant) {
    return mux_error_failure;
  }
  *out_variant_data = *best_variant;
  return mux_success;
}

}  // namespace riscv

mux_result_t riscvCreateBuiltInKernel(mux_device_t device, const char *name,
                                      uint64_t name_length,
                                      mux_allocator_info_t allocator_info,
                                      mux_kernel_t *out_kernel) {
  return mux_error_feature_unsupported;
}

mux_result_t riscvCreateKernel(mux_device_t device, mux_executable_t executable,
                               const char *name, uint64_t name_length,
                               mux_allocator_info_t allocator_info,
                               mux_kernel_t *out_kernel) {
  auto kernel =
      riscv::kernel_s::create(static_cast<riscv::device_s *>(device),
                              static_cast<riscv::executable_s *>(executable),
                              {name, name_length}, allocator_info);
  if (!kernel) {
    return kernel.error();
  }
  *out_kernel = *kernel;
  return mux_success;
}

void riscvDestroyKernel(mux_device_t device, mux_kernel_t kernel,
                        mux_allocator_info_t allocator_info) {
  riscv::kernel_s::destroy(static_cast<riscv::device_s *>(device),
                           static_cast<riscv::kernel_s *>(kernel),
                           allocator_info);
}

mux_result_t riscvQueryMaxNumSubGroups(mux_kernel_t kernel,
                                       size_t *out_max_num_sub_groups) {
  auto *riscv_kernel = static_cast<riscv::kernel_s *>(kernel);
  size_t min_sub_group_size = std::numeric_limits<size_t>::max();

  for (size_t i = 0, e = riscv_kernel->variant_data.size(); i != e; i++) {
    auto variant_sg_size = riscv_kernel->variant_data[i].sub_group_size;
    if (variant_sg_size != 0 && min_sub_group_size > variant_sg_size) {
      min_sub_group_size = variant_sg_size;
    }
  }

  if (min_sub_group_size == std::numeric_limits<size_t>::max()) {
    // If we've found no variant, or a variant using degenerate sub-groups, we
    // only support one sub-group.
    *out_max_num_sub_groups = 1;
  } else {
    // Else we can have as many sub-groups as there are work-items, divided by
    // the smallest sub-group size we've got.
    *out_max_num_sub_groups =
        kernel->device->info->max_concurrent_work_items / min_sub_group_size;
  }

  return mux_success;
}

mux_result_t riscvQuerySubGroupSizeForLocalSize(mux_kernel_t kernel,
                                                size_t local_size_x,
                                                size_t local_size_y,
                                                size_t local_size_z,
                                                size_t *out_sub_group_size) {
  return static_cast<riscv::kernel_s *>(kernel)->getSubGroupSizeForLocalSize(
      local_size_x, local_size_y, local_size_z, out_sub_group_size);
}

mux_result_t riscvQueryWFVInfoForLocalSize(
    mux_kernel_t kernel, size_t local_size_x, size_t local_size_y,
    size_t local_size_z, mux_wfv_status_e *out_wfv_status,
    size_t *out_work_width_x, size_t *out_work_width_y,
    size_t *out_work_width_z) {
  return static_cast<riscv::kernel_s *>(kernel)->getWFVInfoForLocalSize(
      local_size_x, local_size_y, local_size_z, out_wfv_status,
      out_work_width_x, out_work_width_y, out_work_width_z);
}

mux_result_t riscvQueryLocalSizeForSubGroupCount(mux_kernel_t kernel,
                                                 size_t sub_group_count,
                                                 size_t *out_local_size_x,
                                                 size_t *out_local_size_y,
                                                 size_t *out_local_size_z) {
  return static_cast<riscv::kernel_s *>(kernel)->getLocalSizeForSubGroupCount(
      sub_group_count, out_local_size_x, out_local_size_y, out_local_size_z);
}
