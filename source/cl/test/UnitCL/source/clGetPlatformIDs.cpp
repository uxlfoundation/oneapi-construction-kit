// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

TEST(clGetPlatformIDs, Default) {
  cl_uint num_platforms;

  ASSERT_SUCCESS(clGetPlatformIDs(0, nullptr, &num_platforms));
  ASSERT_GT(num_platforms, 0u);

  UCL::Buffer<cl_platform_id> platforms(num_platforms);

  ASSERT_SUCCESS(clGetPlatformIDs(num_platforms, platforms, nullptr));

  for (cl_uint i = 0; i < num_platforms; i++) {
    ASSERT_TRUE(platforms[i]);
  }
}

TEST(clGetPlatformIDs, ZeroPlatformsRequestedWithNonNullPlatforms) {
  cl_platform_id platform;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clGetPlatformIDs(0, &platform, nullptr));
}

TEST(clGetPlatformIDs, PlatformsRequestedWithNullPlatforms) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clGetPlatformIDs(1, nullptr, nullptr));
}

TEST(clGetPlatformIDs, AllValuesNull) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clGetPlatformIDs(0, nullptr, nullptr));
}
