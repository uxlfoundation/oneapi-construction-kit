// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clCreatePipeTest : public ucl::ContextTest {
 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::ContextTest::SetUp());
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_F(clCreatePipeTest, NotImplemented) {
  cl_bool pipe_support{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PIPE_SUPPORT,
                                 sizeof(pipe_support), &pipe_support, nullptr));
  if (CL_FALSE != pipe_support) {
    // Since we test against other implementations that may implement this
    // but we aren't actually testing the functionality, just skip.
    GTEST_SKIP();
  }
  cl_mem_flags flags{};
  cl_uint pipe_packet_size{};
  cl_uint pipe_max_packets{};
  const cl_pipe_properties *properties{};
  cl_int errcode{};
  EXPECT_EQ(clCreatePipe(context, flags, pipe_packet_size, pipe_max_packets,
                         properties, &errcode),
            nullptr);
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, errcode);
}
