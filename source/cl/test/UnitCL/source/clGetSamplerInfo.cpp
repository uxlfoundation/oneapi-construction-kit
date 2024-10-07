// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Common.h"

class clGetSamplerInfoTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    cl_int status;
    sampler = clCreateSampler(context, CL_TRUE, CL_ADDRESS_NONE,
                              CL_FILTER_NEAREST, &status);
    ASSERT_SUCCESS(status);
  }

  void TearDown() override {
    if (sampler) {
      EXPECT_SUCCESS(clReleaseSampler(sampler));
    }
    ContextTest::TearDown();
  }

  cl_sampler sampler = nullptr;
};

TEST_F(clGetSamplerInfoTest, InvalidValueParamName) {
  cl_uint val;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetSamplerInfo(sampler, 0, sizeof(cl_uint), &val, nullptr));
}

TEST_F(clGetSamplerInfoTest, InvalidValueParamValueSize) {
  cl_uint reference_count;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetSamplerInfo(sampler, CL_SAMPLER_REFERENCE_COUNT, sizeof(cl_uint) - 1,
                       &reference_count, nullptr));

  cl_context sampler_context;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetSamplerInfo(sampler, CL_SAMPLER_CONTEXT, sizeof(cl_context) - 1,
                       static_cast<void *>(&sampler_context), nullptr));

  cl_bool normalized_coords;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetSamplerInfo(sampler, CL_SAMPLER_NORMALIZED_COORDS,
                       sizeof(cl_bool) - 1, &normalized_coords, nullptr));

  cl_addressing_mode addressing_mode;
  ASSERT_EQ(CL_INVALID_VALUE,
            clGetSamplerInfo(sampler, CL_SAMPLER_ADDRESSING_MODE,
                             sizeof(cl_addressing_mode) - 1, &addressing_mode,
                             nullptr));

  cl_filter_mode filter_mode;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetSamplerInfo(sampler, CL_SAMPLER_FILTER_MODE,
                       sizeof(cl_filter_mode) - 1, &filter_mode, nullptr));
}

TEST_F(clGetSamplerInfoTest, InvalidSampler) {
  cl_context sampler_context;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_SAMPLER,
      clGetSamplerInfo(nullptr, CL_SAMPLER_CONTEXT, sizeof(cl_context),
                       static_cast<void *>(&sampler_context), nullptr));
}

TEST_F(clGetSamplerInfoTest, DefaultReferenceCount) {
  cl_uint reference_count;
  ASSERT_SUCCESS(clGetSamplerInfo(sampler, CL_SAMPLER_REFERENCE_COUNT,
                                  sizeof(cl_uint), &reference_count, nullptr));
}

TEST_F(clGetSamplerInfoTest, DefaultContext) {
  cl_context sampler_context;
  ASSERT_SUCCESS(
      clGetSamplerInfo(sampler, CL_SAMPLER_CONTEXT, sizeof(cl_context),
                       static_cast<void *>(&sampler_context), nullptr));
  ASSERT_EQ(context, sampler_context);
}

struct sampler_args {
  sampler_args(cl_bool normalized_coords, cl_addressing_mode addressing_mode,
               cl_filter_mode filter_mode)
      : normalized_coords(normalized_coords),
        addressing_mode(addressing_mode),
        filter_mode(filter_mode) {}

  cl_bool normalized_coords;
  cl_addressing_mode addressing_mode;
  cl_filter_mode filter_mode;
};

static std::ostream &operator<<(std::ostream &out, sampler_args params) {
  const std::string normalized_coords =
      params.normalized_coords ? "CL_TRUE" : "CL_FALSE";
  out << "sampler_args{"
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

struct ValueTest : ucl::ContextTest, testing::WithParamInterface<sampler_args> {
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    cl_int status;
    sampler = clCreateSampler(context, GetParam().normalized_coords,
                              GetParam().addressing_mode,
                              GetParam().filter_mode, &status);
    EXPECT_TRUE(sampler);
    ASSERT_SUCCESS(status);
  }

  void TearDown() {
    if (sampler) {
      EXPECT_SUCCESS(clReleaseSampler(sampler));
    }
    ContextTest::TearDown();
  }

  cl_sampler sampler = nullptr;
};

TEST_P(ValueTest, NormalizedCoords) {
  cl_bool normalized_coords;
  ASSERT_SUCCESS(clGetSamplerInfo(sampler, CL_SAMPLER_NORMALIZED_COORDS,
                                  sizeof(cl_bool), &normalized_coords,
                                  nullptr));
  ASSERT_EQ(GetParam().normalized_coords, normalized_coords);
}

TEST_P(ValueTest, AddressingMode) {
  cl_addressing_mode addressing_mode;
  ASSERT_SUCCESS(clGetSamplerInfo(sampler, CL_SAMPLER_ADDRESSING_MODE,
                                  sizeof(cl_addressing_mode), &addressing_mode,
                                  nullptr));
  ASSERT_EQ(GetParam().addressing_mode, addressing_mode);
}

TEST_P(ValueTest, FilterMode) {
  cl_filter_mode filter_mode;
  ASSERT_SUCCESS(clGetSamplerInfo(sampler, CL_SAMPLER_FILTER_MODE,
                                  sizeof(cl_filter_mode), &filter_mode,
                                  nullptr));
  ASSERT_EQ(GetParam().filter_mode, filter_mode);
}

INSTANTIATE_TEST_CASE_P(
    clGetSamplerInfo, ValueTest,
    ::testing::Values(
        sampler_args(CL_TRUE, CL_ADDRESS_MIRRORED_REPEAT, CL_FILTER_NEAREST),
        sampler_args(CL_TRUE, CL_ADDRESS_REPEAT, CL_FILTER_NEAREST),
        sampler_args(CL_TRUE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST),
        sampler_args(CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST),
        sampler_args(CL_TRUE, CL_ADDRESS_NONE, CL_FILTER_NEAREST),
        sampler_args(CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST),
        sampler_args(CL_FALSE, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST),
        sampler_args(CL_FALSE, CL_ADDRESS_NONE, CL_FILTER_NEAREST),
        sampler_args(CL_TRUE, CL_ADDRESS_MIRRORED_REPEAT, CL_FILTER_LINEAR),
        sampler_args(CL_TRUE, CL_ADDRESS_REPEAT, CL_FILTER_LINEAR),
        sampler_args(CL_TRUE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR),
        sampler_args(CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_LINEAR),
        sampler_args(CL_TRUE, CL_ADDRESS_NONE, CL_FILTER_LINEAR),
        sampler_args(CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR),
        sampler_args(CL_FALSE, CL_ADDRESS_CLAMP, CL_FILTER_LINEAR),
        sampler_args(CL_FALSE, CL_ADDRESS_NONE, CL_FILTER_LINEAR)));
