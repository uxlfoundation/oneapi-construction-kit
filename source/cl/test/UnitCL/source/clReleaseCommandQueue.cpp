// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clReleaseCommandQueueTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    queue = clCreateCommandQueue(context, device, 0, &errorcode);
    EXPECT_TRUE(queue);
    ASSERT_SUCCESS(errorcode);
  }

  cl_command_queue queue = nullptr;
};

TEST_F(clReleaseCommandQueueTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE, clReleaseCommandQueue(nullptr));
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}
