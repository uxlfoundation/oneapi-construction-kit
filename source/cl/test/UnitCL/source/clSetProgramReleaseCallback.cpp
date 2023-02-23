// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cstring>

#include "Common.h"

class clSetProgramReleaseCallbackTest : public ucl::ContextTest {
 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::ContextTest::SetUp());
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }

    const char *code = R"(
kernel void test(global int* out) {
  size_t id = get_global_id(0);
  out[id] = (int)id;
}
)";
    const size_t length = std::strlen(code);
    cl_int error;
    program = clCreateProgramWithSource(context, 1, &code, &length, &error);
    ASSERT_SUCCESS(error);
    ASSERT_NE(program, nullptr);
  }

  virtual void TearDown() {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ucl::ContextTest::TearDown();
  }

  cl_program program = nullptr;
};

TEST_F(clSetProgramReleaseCallbackTest, NotImplemented) {
  cl_bool scope_global_ctors_present{};
  ASSERT_SUCCESS(clGetProgramInfo(program,
                                  CL_PROGRAM_SCOPE_GLOBAL_CTORS_PRESENT,
                                  sizeof(scope_global_ctors_present),
                                  &scope_global_ctors_present, nullptr));
  cl_bool scope_global_dtors_present{};
  ASSERT_SUCCESS(clGetProgramInfo(program,
                                  CL_PROGRAM_SCOPE_GLOBAL_DTORS_PRESENT,
                                  sizeof(scope_global_dtors_present),
                                  &scope_global_ctors_present, nullptr));
  if (CL_FALSE != scope_global_ctors_present ||
      CL_FALSE != scope_global_dtors_present) {
    // Since we test against other implementations that may implement this
    // but we aren't actually testing the functionality, just skip.
    GTEST_SKIP();
  }
  void(CL_CALLBACK * pfn_notify)(cl_program, void *){};
  void *user_data{};
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, clSetProgramReleaseCallback(
                                              program, pfn_notify, user_data));
}
