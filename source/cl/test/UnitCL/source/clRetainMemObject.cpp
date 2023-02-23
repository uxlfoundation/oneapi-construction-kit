// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clRetainMemObjectTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    buffer = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
    EXPECT_TRUE(buffer);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    ContextTest::TearDown();
  }

  cl_mem buffer = nullptr;
};

TEST_F(clRetainMemObjectTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT, clRetainMemObject(nullptr));
  ASSERT_SUCCESS(clRetainMemObject(buffer));
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}
