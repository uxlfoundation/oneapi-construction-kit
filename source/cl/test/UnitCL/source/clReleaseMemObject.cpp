// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clReleaseMemObjectTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    buffer = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
    EXPECT_TRUE(buffer);
    ASSERT_SUCCESS(errorcode);
  }

  cl_mem buffer = nullptr;
};

TEST_F(clReleaseMemObjectTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT, clReleaseMemObject(nullptr));
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}
