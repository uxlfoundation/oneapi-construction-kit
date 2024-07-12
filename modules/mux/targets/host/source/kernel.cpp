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

#include <cargo/string_view.h>
#include <host/device.h>
#include <host/executable.h>
#include <host/host.h>
#include <host/kernel.h>
#include <mux/mux.h>

#include <algorithm>
#include <memory>
#include <new>

namespace {
/// @brief Populate preferred local size fields of mux_kernel_s.
///
/// @param[in,out] hostKernel to set size fields on.
inline void setPreferredSizes(host::kernel_s &hostKernel) {
  // These preferred local sizes are fairly arbitrary, at the moment the key
  // point is that they are greater than 1 to ensure that the vectorizer,
  // barrier code, and local work items scheduling are used.  We work best with
  // powers of two.
  hostKernel.preferred_local_size_x =
      std::min(64u, hostKernel.device->info->max_work_group_size_x);
  hostKernel.preferred_local_size_y =
      std::min(4u, hostKernel.device->info->max_work_group_size_y);
  hostKernel.preferred_local_size_z =
      std::min(4u, hostKernel.device->info->max_work_group_size_z);
}
}  // namespace

namespace host {
kernel_variant_s::kernel_variant_s(std::string name, entry_hook_t hook,
                                   size_t local_memory_used,
                                   uint32_t min_work_width,
                                   uint32_t pref_work_width,
                                   uint32_t sub_group_size)
    : name(name),
      hook(hook),
      local_memory_used(local_memory_used),
      min_work_width(min_work_width),
      pref_work_width(pref_work_width),
      sub_group_size(sub_group_size) {}

// Kernel with a built-in kernel
kernel_s::kernel_s(mux_device_t device, mux::allocator allocator,
                   const char *kern_name, size_t name_length,
                   kernel_variant_s::entry_hook_t hook)
    : is_builtin_kernel(true), allocator_info(allocator.getAllocatorInfo()) {
  this->device = device;
  this->local_memory_size = 0;
  auto err = variant_data.push_back(kernel_variant_s{
      std::string(kern_name, name_length), hook, 0u, 1u, 1u, 0u});
  (void)err;
  assert(err == cargo::success);
  setPreferredSizes(*this);
}

// Kernel from a pre-compiled binary
kernel_s::kernel_s(mux_device_t device, mux_allocator_info_t allocator_info,
                   cargo::small_vector<kernel_variant_s, 4> &&variants)
    : is_builtin_kernel(false),
      allocator_info(allocator_info),
      variant_data(std::move(variants)) {
  this->device = device;
  // Just select the maximum local memory size across each variant.
  local_memory_size = 0;
  for (unsigned i = 0, e = variant_data.size(); i != e; i++) {
    local_memory_size =
        std::max(local_memory_size, variant_data[i].local_memory_used);
  }
  setPreferredSizes(*this);
}
}  // namespace host

mux_result_t hostCreateBuiltInKernel(mux_device_t device, const char *name,
                                     uint64_t name_length,
                                     mux_allocator_info_t allocator_info,
                                     mux_kernel_t *out_kernel) {
  (void)name_length;
  auto hostDeviceInfo = static_cast<host::device_info_s *>(device->info);
  mux::allocator allocator(allocator_info);
  const char *builtin_name = nullptr;
  uint64_t builtin_name_length = 0;

  auto kernel_name = std::string{name};
  host::kernel_variant_s::entry_hook_t hook;
  for (auto &builtin_kernel : hostDeviceInfo->builtin_kernel_map) {
    if (builtin_kernel.first.find(kernel_name) != std::string::npos) {
      builtin_name = builtin_kernel.first.data();
      builtin_name_length = builtin_kernel.first.size();
      hook = builtin_kernel.second;
      break;
    }
  }
  if (!builtin_name) {
    return mux_error_invalid_value;
  }

  auto kernel = allocator.create<host::kernel_s>(
      device, allocator_info, builtin_name, builtin_name_length, hook);
  if (nullptr == kernel) {
    return mux_error_out_of_memory;
  }
  *out_kernel = kernel;
  return mux_success;
}

mux_result_t hostCreateKernel(mux_device_t device, mux_executable_t executable,
                              const char *name, uint64_t name_length,
                              mux_allocator_info_t allocator_info,
                              mux_kernel_t *out_kernel) {
  const std::string refName(name, name_length);

  auto hostExecutable = static_cast<host::executable_s *>(executable);
  mux::allocator allocator(allocator_info);
  auto entry = hostExecutable->kernels.find(refName);

  if (entry == hostExecutable->kernels.end()) {
    return mux_error_missing_kernel;
  }

  cargo::small_vector<host::kernel_variant_s, 4> variants;

  for (const auto &v : entry->second) {
    auto err = variants.emplace_back(host::kernel_variant_s{
        std::string(name, name_length),
        reinterpret_cast<host::kernel_variant_s::entry_hook_t>(v.hook),
        v.local_memory_used, v.min_work_width, v.pref_work_width,
        v.sub_group_size});
    if (err != cargo::success) {
      return mux_error_out_of_memory;
    }
  }

  auto kernel = allocator.create<host::kernel_s>(device, allocator_info,
                                                 std::move(variants));
  if (nullptr == kernel) {
    return mux_error_out_of_memory;
  }

  *out_kernel = kernel;

  return mux_success;
}

static bool isLegalKernelVariant(const host::kernel_variant_s &variant,
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

mux_result_t host::kernel_s::getKernelVariantForWGSize(
    size_t local_size_x, size_t local_size_y, size_t local_size_z,
    host::kernel_variant_s *out_variant_data) {
  (void)local_size_y;
  (void)local_size_z;
  host::kernel_variant_s *best_variant = nullptr;
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

mux_result_t hostQuerySubGroupSizeForLocalSize(mux_kernel_t kernel,
                                               size_t local_size_x,
                                               size_t local_size_y,
                                               size_t local_size_z,
                                               size_t *out_sub_group_size) {
  host::kernel_variant_s variant;
  auto host_kernel = static_cast<host::kernel_s *>(kernel);
  auto err = host_kernel->getKernelVariantForWGSize(local_size_x, local_size_y,
                                                    local_size_z, &variant);
  if (err != mux_success) {
    return err;
  }
  // If we've compiled with degenerate sub-groups, the sub-group size is the
  // work-group size.
  if (variant.sub_group_size == 0) {
    *out_sub_group_size = local_size_x * local_size_y * local_size_z;
  } else {
    // Otherwise, on host we always use vectorize in the x-dimension, so
    // sub-groups "go" in the x-dimension.
    *out_sub_group_size =
        std::min(local_size_x, static_cast<size_t>(variant.sub_group_size));
  }
  return mux_success;
}

mux_result_t hostQueryWFVInfoForLocalSize(mux_kernel_t, size_t, size_t, size_t,
                                          mux_wfv_status_e *, size_t *,
                                          size_t *, size_t *) {
  return mux_error_feature_unsupported;
}

mux_result_t hostQueryLocalSizeForSubGroupCount(mux_kernel_t kernel,
                                                size_t sub_group_count,
                                                size_t *local_size_x,
                                                size_t *local_size_y,
                                                size_t *local_size_z) {
  host::kernel_variant_s variant;
  auto host_kernel = static_cast<host::kernel_s *>(kernel);
  const auto &info = *host_kernel->device->info;
  const auto max_local_size_x = info.max_work_group_size_x;
  auto err =
      host_kernel->getKernelVariantForWGSize(max_local_size_x, 1, 1, &variant);
  if (err != mux_success) {
    return err;
  }

  // If we've compiled with degenerate sub-groups, the work-group size is the
  // sub-group size.
  const auto local_size = [&]() -> size_t {
    if (variant.sub_group_size == 0) {
      return sub_group_count == 1 ? max_local_size_x : 0;
    } else {
      const auto local_size = sub_group_count * variant.sub_group_size;
      return local_size <= max_local_size_x ? local_size : 0;
    }
  }();
  if (local_size) {
    *local_size_x = local_size;
    *local_size_y = 1;
    *local_size_z = 1;
  } else {
    *local_size_x = 0;
    *local_size_y = 0;
    *local_size_z = 0;
  }
  return mux_success;
}

mux_result_t hostQueryMaxNumSubGroups(mux_kernel_t kernel,
                                      size_t *out_max_num_sub_groups) {
  auto *host_kernel = static_cast<host::kernel_s *>(kernel);
  size_t min_sub_group_size = std::numeric_limits<size_t>::max();

  for (size_t i = 0, e = host_kernel->variant_data.size(); i != e; i++) {
    auto variant_sg_size = host_kernel->variant_data[i].sub_group_size;
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

void hostDestroyKernel(mux_device_t device, mux_kernel_t kernel,
                       mux_allocator_info_t allocator_info) {
  (void)device;
  mux::allocator allocator(allocator_info);
  auto hostKernel = static_cast<host::kernel_s *>(kernel);

  allocator.destroy(hostKernel);
}
