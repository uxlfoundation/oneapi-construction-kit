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

#include "mux/select.h"
#include "mux/utils/id.h"
#include "tracer/tracer.h"

mux_result_t muxCreateKernel(mux_device_t device, mux_executable_t executable,
                             const char *name, uint64_t name_length,
                             mux_allocator_info_t allocator_info,
                             mux_kernel_t *out_kernel) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(executable)) {
    return mux_error_invalid_value;
  }

  if (!name) {
    return mux_error_invalid_value;
  }

  if (!name_length) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (!out_kernel) {
    return mux_error_null_out_parameter;
  }

  const mux_result_t error = muxSelectCreateKernel(
      device, executable, name, name_length, allocator_info, out_kernel);

  if (mux_success == error) {
    mux::setId<mux_object_id_kernel>(device->id, *out_kernel);
  }

  return error;
}

mux_result_t muxCreateBuiltInKernel(mux_device_t device, const char *name,
                                    uint64_t name_length,
                                    mux_allocator_info_t allocator_info,
                                    mux_kernel_t *out_kernel) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (!name) {
    return mux_error_invalid_value;
  }

  if (!name_length) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (!out_kernel) {
    return mux_error_null_out_parameter;
  }

  const mux_result_t error = muxSelectCreateBuiltInKernel(
      device, name, name_length, allocator_info, out_kernel);

  if (mux_success == error) {
    mux::setId<mux_object_id_kernel>(device->id, *out_kernel);
  }

  return error;
}

mux_result_t muxQuerySubGroupSizeForLocalSize(mux_kernel_t kernel,
                                              size_t local_size_x,
                                              size_t local_size_y,
                                              size_t local_size_z,
                                              size_t *out_sub_group_size) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);
  if (mux::objectIsInvalid(kernel)) {
    return mux_error_invalid_value;
  }

  if (!local_size_x || !local_size_y || !local_size_z) {
    return mux_error_invalid_value;
  }

  if (!out_sub_group_size) {
    return mux_error_null_out_parameter;
  }

  return muxSelectQuerySubGroupSizeForLocalSize(
      kernel, local_size_x, local_size_y, local_size_z, out_sub_group_size);
}

mux_result_t muxQueryWFVInfoForLocalSize(
    mux_kernel_t kernel, size_t local_size_x, size_t local_size_y,
    size_t local_size_z, mux_wfv_status_e *out_wfv_status,
    size_t *out_work_width_x, size_t *out_work_width_y,
    size_t *out_work_width_z) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);
  if (mux::objectIsInvalid(kernel)) {
    return mux_error_invalid_value;
  }

  if (!local_size_x || !local_size_y || !local_size_z) {
    return mux_error_invalid_value;
  }

  if (!out_wfv_status &&
      !(out_work_width_x && out_work_width_y && out_work_width_z)) {
    return mux_error_null_out_parameter;
  }

  return muxSelectQueryWFVInfoForLocalSize(
      kernel, local_size_x, local_size_y, local_size_z, out_wfv_status,
      out_work_width_x, out_work_width_y, out_work_width_z);
}

mux_result_t muxQueryMaxNumSubGroups(mux_kernel_t kernel,
                                     size_t *out_max_sub_group_size) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);
  if (mux::objectIsInvalid(kernel)) {
    return mux_error_invalid_value;
  }

  if (!out_max_sub_group_size) {
    return mux_error_invalid_value;
  }

  return muxSelectQueryMaxNumSubGroups(kernel, out_max_sub_group_size);
}

mux_result_t muxQueryLocalSizeForSubGroupCount(mux_kernel_t kernel,
                                               size_t sub_group_count,
                                               size_t *out_local_size_x,
                                               size_t *out_local_size_y,
                                               size_t *out_local_size_z) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);
  if (mux::objectIsInvalid(kernel)) {
    return mux_error_invalid_value;
  }

  if (!sub_group_count) {
    return mux_error_invalid_value;
  }

  if (!out_local_size_x) {
    return mux_error_null_out_parameter;
  }

  if (!out_local_size_y) {
    return mux_error_null_out_parameter;
  }

  if (!out_local_size_z) {
    return mux_error_null_out_parameter;
  }

  return muxSelectQueryLocalSizeForSubGroupCount(
      kernel, sub_group_count, out_local_size_x, out_local_size_y,
      out_local_size_z);
}

void muxDestroyKernel(mux_device_t device, mux_kernel_t kernel,
                      mux_allocator_info_t allocator_info) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return;
  }

  if (mux::objectIsInvalid(kernel)) {
    return;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return;
  }

  muxSelectDestroyKernel(device, kernel, allocator_info);
}
