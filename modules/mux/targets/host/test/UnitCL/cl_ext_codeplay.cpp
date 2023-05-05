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
