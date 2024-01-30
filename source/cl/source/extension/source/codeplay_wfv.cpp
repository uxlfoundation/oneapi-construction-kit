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

#include <cl/device.h>
#include <cl/kernel.h>
#include <cl/program.h>
#include <extension/codeplay_wfv.h>

namespace wfv {
bool deviceSupportsVectorization(cl_device_id device) {
  return device->compiler_info ? device->compiler_info->vectorizable : false;
}
}  // namespace wfv

extension::codeplay_wfv::codeplay_wfv()
    : extension("cl_codeplay_wfv",
#ifdef OCL_EXTENSION_cl_codeplay_wfv
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}

void *extension::codeplay_wfv::GetExtensionFunctionAddressForPlatform(
    cl_platform_id platform, const char *func_name) const {
  OCL_UNUSED(platform);
#ifndef OCL_EXTENSION_cl_codeplay_wfv
  OCL_UNUSED(func_name);
  return nullptr;
#else
  OCL_CHECK(nullptr == func_name, return nullptr);
  if (0 == strcmp("clGetKernelWFVInfoCODEPLAY", func_name)) {
    return (void *)&clGetKernelWFVInfoCODEPLAY;
  }

  return nullptr;
#endif
}

cl_int extension::codeplay_wfv::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
#ifndef OCL_EXTENSION_cl_codeplay_wfv
  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
#else
  // Vectorization support is required to support the extension, rather than an
  // optional capability. Therefore, if the extension is enabled in the build
  // but a device doesn't have this capability, then return CL_INVALID_DEVICE
  // to our CL extension mechanism so it knows not to include the extension
  // when queried by the user for CL_DEVICE_EXTENSIONS.
  if (!wfv::deviceSupportsVectorization(device)) {
    return CL_INVALID_DEVICE;
  } else {
    return extension::GetDeviceInfo(device, param_name, param_value_size,
                                    param_value, param_value_size_ret);
  }
#endif
}

cl_int CL_API_CALL clGetKernelWFVInfoCODEPLAY(
    cl_kernel kernel, cl_device_id device, cl_uint work_dim,
    const size_t *global_work_size, const size_t *local_work_size,
    cl_kernel_work_group_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
#ifndef OCL_EXTENSION_cl_codeplay_wfv
  OCL_UNUSED(kernel);
  OCL_UNUSED(device);
  OCL_UNUSED(work_dim);
  OCL_UNUSED(global_work_size);
  OCL_UNUSED(local_work_size);
  OCL_UNUSED(param_name);
  OCL_UNUSED(param_value_size);
  OCL_UNUSED(param_value);
  OCL_UNUSED(param_value_size_ret);
  return CL_INVALID_OPERATION;
#else
  if (kernel == nullptr) {
    return CL_INVALID_KERNEL;
  }

  if (device == nullptr) {
    if (kernel->program->programs.size() != 1) {
      return CL_INVALID_DEVICE;
    } else {
      device = kernel->program->programs.begin()->first;
    }
  } else {
    auto it = kernel->program->programs.find(device);
    if (it == kernel->program->programs.end()) {
      return CL_INVALID_DEVICE;
    }
  }

  if (work_dim < 1 || work_dim > device->max_work_item_dimensions) {
    return CL_INVALID_WORK_DIMENSION;
  }

  // Check the required work group size (if it exists).
  if (auto error = kernel->checkReqdWorkGroupSize(work_dim, local_work_size)) {
    return error;
  }

  if (auto error = kernel->checkWorkSizes(device, work_dim, nullptr,
                                          global_work_size, local_work_size)) {
    return error;
  }

  std::array<size_t, cl::max::WORK_ITEM_DIM> final_local_work_size;
  final_local_work_size.fill(1);

  if (local_work_size) {
    std::copy_n(local_work_size, work_dim, std::begin(final_local_work_size));
  } else {
    final_local_work_size =
        kernel->getDefaultLocalSize(device, global_work_size, work_dim);
  }

  size_t size;

  switch (param_name) {
    default:
      return CL_INVALID_VALUE;
    case CL_KERNEL_WFV_STATUS_CODEPLAY:
      size = sizeof(cl_kernel_wfv_status_codeplay);
      break;
    case CL_KERNEL_WFV_WIDTHS_CODEPLAY:
      size = sizeof(size_t) * work_dim;
      break;
  }

  if (param_value != nullptr) {
    if (param_value_size < size) {
      return CL_INVALID_VALUE;
    }

    cl_context context = kernel->program->context;

    // The maximum work width represents the number of work-items the kernel
    // will be able to handle per invocation, after whole-function vectorization
    // has been performed.
    //
    // We retrieve the work width within the following scope in order to
    // release the lock guard immediately, so that it is not held for the
    // remainder of this function.
    const uint32_t max_work_width = [&] {
      const std::lock_guard<std::mutex> context_guard(context->mutex);
      return kernel->device_kernel_map[device]->getDynamicWorkWidth(
          final_local_work_size[0], final_local_work_size[1],
          final_local_work_size[2]);
    }();

    switch (param_name) {
      case CL_KERNEL_WFV_STATUS_CODEPLAY: {
        cl_kernel_wfv_status_codeplay *result =
            static_cast<cl_kernel_wfv_status_codeplay *>(param_value);
        if (max_work_width > 1) {
          *result = CL_WFV_SUCCESS_CODEPLAY;
        } else {
          *result = CL_WFV_NONE_CODEPLAY;
        }
        break;
      }
      case CL_KERNEL_WFV_WIDTHS_CODEPLAY: {
        size_t *result = static_cast<size_t *>(param_value);
        for (size_t i = 0; i < work_dim; ++i) {
          result[i] = 1;
        }

        result[0] = max_work_width;
        break;
      }
    }
  }

  if (param_value_size_ret != nullptr) {
    *param_value_size_ret = size;
  }

  return CL_SUCCESS;
#endif
}
