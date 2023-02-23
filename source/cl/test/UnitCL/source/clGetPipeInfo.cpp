// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clGetPipeInfoTest : public ucl::ContextTest {
 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int error{};
    buffer = clCreateBuffer(context, 0, 42, nullptr, &error);
    ASSERT_NE(buffer, nullptr);
    ASSERT_SUCCESS(error);
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }

  virtual void TearDown() {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    ContextTest::TearDown();
  }

  cl_mem buffer = nullptr;
};

TEST_F(clGetPipeInfoTest, NotImplemented) {
  cl_bool pipe_support{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PIPE_SUPPORT,
                                 sizeof(pipe_support), &pipe_support, nullptr));
  if (CL_FALSE != pipe_support) {
    // Since we test against other implementations that may implement this
    // but we aren't actually testing the functionality, just skip.
    GTEST_SKIP();
  }
  cl_pipe_info param_name{};
  size_t param_value_size{};
  void *param_value{};
  size_t *param_value_size_ret{};
  EXPECT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clGetPipeInfo(buffer, param_name, param_value_size,
                                  param_value, param_value_size_ret));
}
