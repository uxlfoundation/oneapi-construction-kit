/// Copyright (C) Codeplay Software Limited. All Rights Reserved.
{% if cookiecutter.copyright_name != "" -%}
/// Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
/// Reserved.
{% endif -%}

#include "{{cookiecutter.target_name}}/kernel.h"

#include <limits>

#include "{{cookiecutter.target_name}}/device.h"
#include "{{cookiecutter.target_name}}/executable.h"

namespace {{cookiecutter.target_name}} {
kernel_s::kernel_s(
    mux::hal::device *device, cargo::string_view name,
    cargo::array_view<uint8_t> object_code, mux::allocator allocator,
    cargo::small_vector<mux::hal::kernel_variant_s, 4> &&variant_data)
    : mux::hal::kernel<mux::hal::kernel_variant_s>(
        device, name, object_code, allocator, std::move(variant_data)) {}

cargo::expected<{{cookiecutter.target_name}}::kernel_s *, mux_result_t> kernel_s::create(
    {{cookiecutter.target_name}}::device_s *device, {{cookiecutter.target_name}}::executable_s *executable,
    cargo::string_view name, mux::allocator allocator) {
  {% if cookiecutter.vlen != 1 -%}
  unsigned real_vscale = {{cookiecutter.vlen}} / 64;
  {% else -%}
  unsigned real_vscale = 1;
  {% endif -%}

  cargo::small_vector<mux::hal::kernel_variant_s, 4> variants;
  for (auto &meta : executable->kernel_info) {
    if (name != meta.source_name) {
      continue;
    }
    mux::hal::kernel_variant_s variant;
    variant.variant_name = meta.kernel_name;
    variant.sub_group_size = meta.sub_group_size.getKnownMinValue();
    if (meta.sub_group_size.isScalable()) {
      variant.sub_group_size *= real_vscale;
    }
    variant.min_work_width = meta.min_work_item_factor.getKnownMinValue();
    if (meta.min_work_item_factor.isScalable()) {
      variant.min_work_width *= real_vscale;
    }
    variant.pref_work_width = meta.pref_work_item_factor.getKnownMinValue();
    if (meta.pref_work_item_factor.isScalable()) {
      variant.pref_work_width *= real_vscale;
    }
    if (cargo::success != variants.push_back(std::move(variant))) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
  }

  if (variants.empty()) {
    return cargo::make_unexpected(mux_error_missing_kernel);
  }

  auto kernel = mux::hal::kernel<mux::hal::kernel_variant_s>::create<{{cookiecutter.target_name}}::kernel_s>(
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
    // Otherwise, on {{cookiecutter.target_name}} we always use vectorize in the x-dimension, so
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
  // FIXME: For a single sub-group, we know we can satisfy that with a
  // work-group of 1,1,1. For any other sub-group count, we should ensure that
  // the work-group size we report comes back through getKernelVariantForWGSize
  // when it comes to run it. See CA-4784.
  if (sub_group_count == 0) {
    *out_local_size_x = 1;
    *out_local_size_y = 1;
    *out_local_size_z = 1;
  } else {
    *out_local_size_x = 0;
    *out_local_size_y = 0;
    *out_local_size_z = 0;
  }
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
  (void)local_size_y;
  (void)local_size_z;
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
}  // namespace {{cookiecutter.target_name}}

mux_result_t {{cookiecutter.target_name}}CreateBuiltInKernel(mux_device_t device, const char *name,
                                      uint64_t name_length,
                                      mux_allocator_info_t allocator_info,
                                      mux_kernel_t *out_kernel) {
  return mux_error_feature_unsupported;
}

mux_result_t {{cookiecutter.target_name}}CreateKernel(mux_device_t device, mux_executable_t executable,
                               const char *name, uint64_t name_length,
                               mux_allocator_info_t allocator_info,
                               mux_kernel_t *out_kernel) {
  auto kernel =
      {{cookiecutter.target_name}}::kernel_s::create(static_cast<{{cookiecutter.target_name}}::device_s *>(device),
                              static_cast<{{cookiecutter.target_name}}::executable_s *>(executable),
                              {name, name_length}, allocator_info);
  if (!kernel) {
    return kernel.error();
  }
  *out_kernel = *kernel;
  return mux_success;
}

void {{cookiecutter.target_name}}DestroyKernel(mux_device_t device, mux_kernel_t kernel,
                        mux_allocator_info_t allocator_info) {
  {{cookiecutter.target_name}}::kernel_s::destroy(static_cast<{{cookiecutter.target_name}}::device_s *>(device),
                           static_cast<{{cookiecutter.target_name}}::kernel_s *>(kernel),
                           allocator_info);
}

mux_result_t {{cookiecutter.target_name}}QuerySubGroupSizeForLocalSize(mux_kernel_t kernel,
                                                size_t local_size_x,
                                                size_t local_size_y,
                                                size_t local_size_z,
                                                size_t *out_sub_group_size) {
  return static_cast<{{cookiecutter.target_name}}::kernel_s *>(kernel)->getSubGroupSizeForLocalSize(
      local_size_x, local_size_y, local_size_z, out_sub_group_size);
}


mux_result_t {{cookiecutter.target_name}}QueryMaxNumSubGroups(mux_kernel_t kernel,
                                           size_t *out_max_num_sub_groups) {
  auto *tgt_kernel = static_cast<{{cookiecutter.target_name}}::kernel_s *>(kernel);
  size_t min_sub_group_size = std::numeric_limits<size_t>::max();

  for (size_t i = 0, e = tgt_kernel->variant_data.size(); i != e; i++) {
    auto variant_sg_size = tgt_kernel->variant_data[i].sub_group_size;
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

mux_result_t {{cookiecutter.target_name}}QueryWFVInfoForLocalSize(
    mux_kernel_t kernel, size_t local_size_x, size_t local_size_y,
    size_t local_size_z, mux_wfv_status_e *out_wfv_status,
    size_t *out_work_width_x, size_t *out_work_width_y,
    size_t *out_work_width_z) {
  return static_cast<{{cookiecutter.target_name}}::kernel_s *>(kernel)->getWFVInfoForLocalSize(
      local_size_x, local_size_y, local_size_z, out_wfv_status,
      out_work_width_x, out_work_width_y, out_work_width_z);
}

mux_result_t {{cookiecutter.target_name}}QueryLocalSizeForSubGroupCount(mux_kernel_t kernel,
                                                 size_t sub_group_count,
                                                 size_t *out_local_size_x,
                                                 size_t *out_local_size_y,
                                                 size_t *out_local_size_z) {
  return static_cast<{{cookiecutter.target_name}}::kernel_s *>(kernel)->getLocalSizeForSubGroupCount(
      sub_group_count, out_local_size_x, out_local_size_y, out_local_size_y);
}
