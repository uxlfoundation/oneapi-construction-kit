// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clReleaseSamplerTest : public ucl::ContextTest {
 protected:
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
  }
};

TEST_F(clReleaseSamplerTest, InvalidSampler) {
  ASSERT_EQ_ERRCODE(CL_INVALID_SAMPLER, clReleaseSampler(nullptr));
}

TEST_F(clReleaseSamplerTest, Default) {
  cl_int status;
  cl_sampler sampler = clCreateSampler(context, CL_FALSE, CL_ADDRESS_NONE,
                                       CL_FILTER_NEAREST, &status);
  EXPECT_TRUE(sampler);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(clReleaseSampler(sampler));
}
