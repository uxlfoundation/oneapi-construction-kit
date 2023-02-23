// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/string_view.h>

#include "Common.h"
#include "Device.h"

// CA host extension header
#include <CL/cl_ext_codeplay_host.h>

// Tests for all extensions in ca_ext_codeplay library
struct cl_ext_codeplay_Test : ucl::DeviceTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    // Since these are host specific test we want to skip it if we aren't
    // running on host.
    if (!UCL::isDevice_host(device)) {
      GTEST_SKIP();
    }
    if (!isDeviceExtensionSupported("cl_codeplay_set_threads")) {
      GTEST_SKIP();
    }
    clSetNumThreadsCODEPLAY = reinterpret_cast<clSetNumThreadsCODEPLAY_fn>(
        clGetExtensionFunctionAddressForPlatform(platform,
                                                 "clSetNumThreadsCODEPLAY"));
    ASSERT_NE(nullptr, clSetNumThreadsCODEPLAY);
  }

  clSetNumThreadsCODEPLAY_fn clSetNumThreadsCODEPLAY = nullptr;
};

TEST_F(cl_ext_codeplay_Test, clSetNumThreadsCODEPLAY) {
  // TODO: CA-1136 -- update when clSetNumThreadsCODEPLAY() is implemented
  ASSERT_EQ_ERRCODE(CL_DEVICE_NOT_AVAILABLE,
                    clSetNumThreadsCODEPLAY(device, 1));
}

TEST_F(cl_ext_codeplay_Test, clSetNumThreadsCODEPLAY_InvalidNumThreads) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clSetNumThreadsCODEPLAY(device, 0));
}

TEST_F(cl_ext_codeplay_Test, clSetNumThreadsCODEPLAY_NullDevice) {
  // TODO: Why not CL_INVALID_DEVICE?
  ASSERT_EQ_ERRCODE(CL_INVALID_DEVICE_TYPE,
                    clSetNumThreadsCODEPLAY(nullptr, 1));
}
