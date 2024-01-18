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

#include <cl/command_queue.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/validate.h>
#include <extension/khr_create_command_queue.h>
#include <tracer/tracer.h>
#include <utils/system.h>

#include <memory>

extension::khr_create_command_queue::khr_create_command_queue()
    : extension("cl_khr_create_command_queue",
#ifdef OCL_EXTENSION_cl_khr_create_command_queue
                usage_category::PLATFORM
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(1, 0, 0)) {
}

CL_API_ENTRY cl_command_queue CL_API_CALL clCreateCommandQueueWithPropertiesKHR(
    cl_context context, cl_device_id device,
    const cl_queue_properties_khr *properties, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> trace(
      "clCreateCommandQueueWithPropertiesKHR");

  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  OCL_CHECK(!device || !context->hasDevice(device),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_DEVICE);
            return nullptr);

  auto device_index = context->getDeviceIndex(device);
  mux_queue_t mux_queue;
  const mux_result_t error =
      muxGetQueue(context->devices[device_index]->mux_device,
                  mux_queue_type_compute, 0, &mux_queue);
  OCL_CHECK(error, OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
            return nullptr);

  auto command_queue = _cl_command_queue::create(context, device, properties);

  if (!command_queue) {
    OCL_SET_IF_NOT_NULL(errcode_ret, command_queue.error());
    return nullptr;
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return command_queue->release();
}

void *
extension::khr_create_command_queue::GetExtensionFunctionAddressForPlatform(
    cl_platform_id platform, const char *func_name) const {
  OCL_UNUSED(platform);
#ifndef OCL_EXTENSION_cl_khr_create_command_queue
  OCL_UNUSED(func_name);
#else
  if (func_name &&
      (std::strcmp("clCreateCommandQueueWithPropertiesKHR", func_name) == 0)) {
    return (void *)&clCreateCommandQueueWithPropertiesKHR;
  }
#endif
  return nullptr;
}
