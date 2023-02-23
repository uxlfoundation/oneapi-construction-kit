// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clReleaseProgramTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    static const char *source = "something";
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
  }

  cl_program program = nullptr;
};

TEST_F(clReleaseProgramTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_PROGRAM, clReleaseProgram(nullptr));
  ASSERT_SUCCESS(clReleaseProgram(program));
}
