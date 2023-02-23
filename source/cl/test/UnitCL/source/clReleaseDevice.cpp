// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

using clReleaseDeviceTest = ucl::DeviceTest;

TEST_F(clReleaseDeviceTest, Default) {
  ASSERT_SUCCESS(clRetainDevice(device));
  ASSERT_SUCCESS(clReleaseDevice(device));
}

TEST_F(clReleaseDeviceTest, BadDevice) {
  ASSERT_EQ_ERRCODE(CL_INVALID_DEVICE, clRetainDevice(nullptr));
}
