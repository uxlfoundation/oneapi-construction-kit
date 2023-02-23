// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clRetainProgramTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    static const char *source = "something";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
};

TEST_F(clRetainProgramTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_PROGRAM, clRetainProgram(nullptr));
  ASSERT_SUCCESS(clRetainProgram(program));
  ASSERT_SUCCESS(clReleaseProgram(program));
}
