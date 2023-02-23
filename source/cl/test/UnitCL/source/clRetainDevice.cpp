// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

using clRetainDeviceTest = ucl::DeviceTest;

TEST_F(clRetainDeviceTest, Default) {
  ASSERT_SUCCESS(clRetainDevice(device));
  ASSERT_SUCCESS(clReleaseDevice(device));
}

TEST_F(clRetainDeviceTest, BadDevice) {
  ASSERT_EQ_ERRCODE(CL_INVALID_DEVICE, clRetainDevice(nullptr));
}
