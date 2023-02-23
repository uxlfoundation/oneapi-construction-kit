// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#ifndef UNITCL_MUTABLE_DISPATCH_H_INCLUDED
#define UNITCL_MUTABLE_DISPATCH_H_INCLUDED

#include "cl_khr_command_buffer.h"

// Base class for checking if the command buffer extension is enabled. If so
// SetUp queries for function pointers to new extension entry points that test
// fixtures can use.
struct MutableDispatchTest : public cl_khr_command_buffer_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());
    // Check whether the extension is supported on this device.
    if (!UCL::hasDeviceExtensionSupport(
            device, "cl_khr_command_buffer_mutable_dispatch")) {
      GTEST_SKIP();
    }

    // All tests update kernel args so require a compiler to compile the
    // kernel.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    cl_mutable_dispatch_fields_khr mutable_capabilities;
    cl_int err = clGetDeviceInfo(
        device, CL_DEVICE_MUTABLE_DISPATCH_CAPABILITIES_KHR,
        sizeof(cl_mutable_dispatch_fields_khr), &mutable_capabilities, nullptr);
    ASSERT_SUCCESS(err);

    // We assume if a device supports this extension then it can do argument
    // update, otherwise skip tests
    if (!(CL_MUTABLE_DISPATCH_ARGUMENTS_KHR & mutable_capabilities)) {
      GTEST_SKIP();
    }

    // If it is supported get the addresses of all the APIs here.
#define GET_EXTENSION_ADDRESS(FUNC)                               \
  FUNC = reinterpret_cast<FUNC##_fn>(                             \
      clGetExtensionFunctionAddressForPlatform(platform, #FUNC)); \
  ASSERT_NE(nullptr, FUNC) << "Could not get address of " << #FUNC;

    GET_EXTENSION_ADDRESS(clUpdateMutableCommandsKHR);
    GET_EXTENSION_ADDRESS(clGetMutableCommandInfoKHR);

#undef GET_EXTENSION_ADDRESS
  }

  clUpdateMutableCommandsKHR_fn clUpdateMutableCommandsKHR = nullptr;
  clGetMutableCommandInfoKHR_fn clGetMutableCommandInfoKHR = nullptr;
};
#endif  // UNITCL_MUTABLE_DISPATCH_H_INCLUDED
