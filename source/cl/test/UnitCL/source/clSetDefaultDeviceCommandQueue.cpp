// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clSetDefaultDeviceCommandQueueTest : public ucl::ContextTest {
 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::ContextTest::SetUp());
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_F(clSetDefaultDeviceCommandQueueTest, NotImplemented) {
  cl_device_device_enqueue_capabilities device_enqueue_support{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES,
                                 sizeof(device_enqueue_support),
                                 &device_enqueue_support, nullptr));
  if (device_enqueue_support != 0) {
    // Since we test against other implementations that may implement this
    // but we aren't actually testing the functionality, just skip.
    GTEST_SKIP();
  }
  cl_command_queue command_queue{};
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, clSetDefaultDeviceCommandQueue(
                                              context, device, command_queue));
}
