// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/string_view.h>

#include "Common.h"
#include "Device.h"

// This is a test simply to test the integeration of external files to UnitCL.
using ExampleUnitCLTest_Host = ucl::DeviceTest;

TEST_F(ExampleUnitCLTest_Host, Default) {
  // Since these are host specific test we want to skip it if we aren't
  // running on host.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP();
  }
  ASSERT_EQ(true, sizeof(size_t) == sizeof(size_t));
}
