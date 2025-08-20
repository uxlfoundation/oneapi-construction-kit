// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef UNITCL_COMMAND_BUFFER_H_INCLUDED
#define UNITCL_COMMAND_BUFFER_H_INCLUDED

#include <CL/cl_ext.h>

#include "Common.h"
#include "kts/precision.h"

// Base class for checking if the command buffer extension is enabled. If so
// SetUp queries for function pointers to new extension entry points that test
// fixtures can use.
struct cl_khr_command_buffer_Test : public ucl::CommandQueueTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    // Check whether the extension is supported on this device.
    if (!UCL::hasDeviceExtensionSupport(device, "cl_khr_command_buffer")) {
      GTEST_SKIP();
    }

    // If it is supported get the addresses of all the APIs here.
#define GET_EXTENSION_ADDRESS(FUNC)                               \
  FUNC = reinterpret_cast<FUNC##_fn>(                             \
      clGetExtensionFunctionAddressForPlatform(platform, #FUNC)); \
  ASSERT_NE(nullptr, FUNC) << "Could not get address of " << #FUNC;

    GET_EXTENSION_ADDRESS(clCreateCommandBufferKHR);
    GET_EXTENSION_ADDRESS(clReleaseCommandBufferKHR);
    GET_EXTENSION_ADDRESS(clRetainCommandBufferKHR);
    GET_EXTENSION_ADDRESS(clFinalizeCommandBufferKHR);
    GET_EXTENSION_ADDRESS(clEnqueueCommandBufferKHR);
    GET_EXTENSION_ADDRESS(clCommandBarrierWithWaitListKHR);
    GET_EXTENSION_ADDRESS(clCommandCopyBufferKHR);
    GET_EXTENSION_ADDRESS(clCommandCopyBufferRectKHR);
    GET_EXTENSION_ADDRESS(clCommandCopyBufferToImageKHR);
    GET_EXTENSION_ADDRESS(clCommandCopyImageKHR);
    GET_EXTENSION_ADDRESS(clCommandCopyImageToBufferKHR);
    GET_EXTENSION_ADDRESS(clCommandFillBufferKHR);
    GET_EXTENSION_ADDRESS(clCommandFillImageKHR);
    GET_EXTENSION_ADDRESS(clCommandNDRangeKernelKHR);
    GET_EXTENSION_ADDRESS(clGetCommandBufferInfoKHR);

#undef GET_EXTENSION_ADDRESS
    // Query device for supported command-buffer capabilities
    cl_int err =
        clGetDeviceInfo(device, CL_DEVICE_COMMAND_BUFFER_CAPABILITIES_KHR,
                        sizeof(cl_device_command_buffer_capabilities_khr),
                        &capabilities, nullptr);
    ASSERT_SUCCESS(err);

    cl_command_queue_properties required_properties;
    err = clGetDeviceInfo(
        device, CL_DEVICE_COMMAND_BUFFER_REQUIRED_QUEUE_PROPERTIES_KHR,
        sizeof(required_properties), &required_properties, nullptr);
    ASSERT_SUCCESS(err);

    // Tests are written assuming no command-queue properties are needed, which
    // is the case for our ComputeAorta implementation.
    if (required_properties != 0) {
      GTEST_SKIP();
    }
  }

  clCreateCommandBufferKHR_fn clCreateCommandBufferKHR = nullptr;
  clReleaseCommandBufferKHR_fn clReleaseCommandBufferKHR = nullptr;
  clRetainCommandBufferKHR_fn clRetainCommandBufferKHR = nullptr;
  clFinalizeCommandBufferKHR_fn clFinalizeCommandBufferKHR = nullptr;
  clEnqueueCommandBufferKHR_fn clEnqueueCommandBufferKHR = nullptr;
  clCommandBarrierWithWaitListKHR_fn clCommandBarrierWithWaitListKHR = nullptr;
  clCommandCopyBufferKHR_fn clCommandCopyBufferKHR = nullptr;
  clCommandCopyBufferRectKHR_fn clCommandCopyBufferRectKHR = nullptr;
  clCommandCopyBufferToImageKHR_fn clCommandCopyBufferToImageKHR = nullptr;
  clCommandCopyImageKHR_fn clCommandCopyImageKHR = nullptr;
  clCommandCopyImageToBufferKHR_fn clCommandCopyImageToBufferKHR = nullptr;
  clCommandFillBufferKHR_fn clCommandFillBufferKHR = nullptr;
  clCommandFillImageKHR_fn clCommandFillImageKHR = nullptr;
  clCommandNDRangeKernelKHR_fn clCommandNDRangeKernelKHR = nullptr;
  clGetCommandBufferInfoKHR_fn clGetCommandBufferInfoKHR = nullptr;

  cl_device_command_buffer_capabilities_khr capabilities = 0;
};
#endif  // UNITCL_COMMAND_BUFFER_H_INCLUDED
