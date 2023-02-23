// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clReleaseContextTest : public ucl::DeviceTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    cl_int errorcode;
    context =
        clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errorcode);
    EXPECT_TRUE(context);
    ASSERT_SUCCESS(errorcode);
  }

  cl_context context = nullptr;
};

TEST_F(clReleaseContextTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_CONTEXT, clReleaseContext(nullptr));
  ASSERT_SUCCESS(clReleaseContext(context));
}
