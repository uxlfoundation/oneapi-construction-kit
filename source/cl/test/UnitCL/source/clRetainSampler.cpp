// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clRetainSamplerTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
  }
};

TEST_F(clRetainSamplerTest, InvalidSampler) {
  ASSERT_EQ_ERRCODE(CL_INVALID_SAMPLER, clRetainSampler(nullptr));
}

TEST_F(clRetainSamplerTest, Default) {
  cl_int status;
  cl_sampler sampler = clCreateSampler(context, CL_FALSE, CL_ADDRESS_NONE,
                                       CL_FILTER_NEAREST, &status);
  EXPECT_TRUE(sampler);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(clRetainSampler(sampler));
  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clReleaseSampler(sampler));  // reverse the retain
  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clReleaseSampler(sampler));  // make ref count 0
}
