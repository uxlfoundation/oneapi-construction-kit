// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

struct sampler_params {
  sampler_params(cl_bool normalized_coords, cl_addressing_mode addressing_mode,
                 cl_filter_mode filter_mode)
      : normalized_coords(normalized_coords),
        addressing_mode(addressing_mode),
        filter_mode(filter_mode) {}

  cl_bool normalized_coords;
  cl_addressing_mode addressing_mode;
  cl_filter_mode filter_mode;
};

std::ostream &operator<<(std::ostream &out, sampler_params params) {
  std::string normalized_coords =
      params.normalized_coords ? "CL_TRUE" : "CL_FALSE";
  out << "sampler_params{"
      << ".normalized_coords{" << normalized_coords << "}, "
      << ".addressing_mode{";
  switch (params.addressing_mode) {
#define CASE(MODE) \
  case MODE:       \
    out << #MODE;  \
    break;
    CASE(CL_ADDRESS_NONE)
    CASE(CL_ADDRESS_CLAMP_TO_EDGE)
    CASE(CL_ADDRESS_CLAMP)
    CASE(CL_ADDRESS_REPEAT)
    CASE(CL_ADDRESS_MIRRORED_REPEAT)
#undef CASE
    default:
      out << "UNKNOWN";
      break;
  }
  out << "}, .filter_mode{";
  switch (params.filter_mode) {
#define CASE(MODE) \
  case MODE:       \
    out << #MODE;  \
    break;
    CASE(CL_FILTER_NEAREST)
    CASE(CL_FILTER_LINEAR)
#undef CASE
    default:
      out << "UNKNOWN";
      break;
  }
  out << "}}";
  return out;
}

struct SamplerDefault : ucl::ContextTest,
                        testing::WithParamInterface<sampler_params> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
  }

  cl_sampler sampler = nullptr;
};

TEST_P(SamplerDefault, Default) {
  cl_int status = !CL_SUCCESS;
  sampler = clCreateSampler(context, GetParam().normalized_coords,
                            GetParam().addressing_mode, GetParam().filter_mode,
                            &status);
  EXPECT_SUCCESS(status);
  ASSERT_SUCCESS(clReleaseSampler(sampler));
}

INSTANTIATE_TEST_CASE_P(
    clCreateSampler, SamplerDefault,
    ::testing::Values(
        sampler_params(CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST),
        sampler_params(CL_FALSE, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST),
        sampler_params(CL_FALSE, CL_ADDRESS_NONE, CL_FILTER_NEAREST),
        sampler_params(CL_TRUE, CL_ADDRESS_MIRRORED_REPEAT, CL_FILTER_NEAREST),
        sampler_params(CL_TRUE, CL_ADDRESS_REPEAT, CL_FILTER_NEAREST),
        sampler_params(CL_TRUE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST),
        sampler_params(CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST),
        sampler_params(CL_TRUE, CL_ADDRESS_NONE, CL_FILTER_NEAREST),
        sampler_params(CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR),
        sampler_params(CL_FALSE, CL_ADDRESS_CLAMP, CL_FILTER_LINEAR),
        sampler_params(CL_FALSE, CL_ADDRESS_NONE, CL_FILTER_LINEAR),
        sampler_params(CL_TRUE, CL_ADDRESS_MIRRORED_REPEAT, CL_FILTER_LINEAR),
        sampler_params(CL_TRUE, CL_ADDRESS_REPEAT, CL_FILTER_LINEAR),
        sampler_params(CL_TRUE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR),
        sampler_params(CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_LINEAR),
        sampler_params(CL_TRUE, CL_ADDRESS_NONE, CL_FILTER_LINEAR)));

struct clCreateSamplerTest : ucl::ContextTest {
  cl_sampler sampler = nullptr;
};

TEST_F(clCreateSamplerTest, Default) {
  if (getDeviceImageSupport()) {
    cl_int status;
    sampler = clCreateSampler(context, CL_TRUE, CL_ADDRESS_NONE,
                              CL_FILTER_NEAREST, &status);
    EXPECT_TRUE(sampler);
    EXPECT_SUCCESS(status);
    ASSERT_EQ(CL_SUCCESS, clReleaseSampler(sampler));
  } else {
    cl_int status;
    sampler = clCreateSampler(context, CL_TRUE, CL_ADDRESS_NONE,
                              CL_FILTER_NEAREST, &status);
    EXPECT_FALSE(sampler);
    ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION, status);
  }
}

TEST_F(clCreateSamplerTest, DefaultUsage) {
  if (!getDeviceImageSupport()) {
    GTEST_SKIP();
  }
  // Redmine #5118: Run a kernel with a sampler.
}

TEST_F(clCreateSamplerTest, InvalidContext) {
  if (!getDeviceImageSupport()) {
    GTEST_SKIP();
  }
  cl_int status;
  sampler = clCreateSampler(nullptr, CL_TRUE, CL_ADDRESS_NONE,
                            CL_FILTER_NEAREST, &status);
  EXPECT_FALSE(sampler);
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, status);
}

TEST_F(clCreateSamplerTest, InvalidValueAddressingMode) {
  if (!getDeviceImageSupport()) {
    GTEST_SKIP();
  }
  cl_int status;
  sampler = clCreateSampler(context, CL_TRUE, 0, CL_FILTER_NEAREST, &status);
  EXPECT_FALSE(sampler);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
}

TEST_F(clCreateSamplerTest, InvalidValueNormalizedCoords) {
  if (!getDeviceImageSupport()) {
    GTEST_SKIP();
  }
  cl_int status;
  sampler = clCreateSampler(context, CL_TRUE, CL_ADDRESS_NONE, 0, &status);
  EXPECT_FALSE(sampler);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
}

// Redmine #5117: Check CL_OUT_OF_RESOURCES
// Redmine #5114: Check CL_OUT_OF_HOST_MEMORY
