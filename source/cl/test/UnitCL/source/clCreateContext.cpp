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

#include <thread>

#include "Common.h"

struct clCreateContextTest
    : ucl::DeviceTest,
      testing::WithParamInterface<cl_context_properties> {};

TEST_F(clCreateContextTest, Default) {
  cl_int errcode;
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errcode);
  ASSERT_TRUE(context);
  EXPECT_SUCCESS(errcode);

  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clCreateContextTest, DefaultCallback) {
  struct callback_data_t {
  } callback_data;
  auto callback = [](const char *, const void *, size_t, void *)
                      CL_LAMBDA_CALLBACK {};
  cl_int error;
  cl_context context =
      clCreateContext(nullptr, 1, &device, callback, &callback_data, &error);
  ASSERT_SUCCESS(error);
  ASSERT_NE(nullptr, context);
  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clCreateContextTest, nullptrErrorCode) {
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
  ASSERT_TRUE(context);
  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clCreateContextTest, WithBadProperties) {
  cl_int errcode;
  cl_context_properties properties[2];
  properties[0] = 1;
  properties[1] = 0;
  EXPECT_FALSE(
      clCreateContext(properties, 1, &device, nullptr, nullptr, &errcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_PROPERTY, errcode);
}

TEST_F(clCreateContextTest, INTEROP_USER_SYNC) {
  cl_context_properties properties[3] = {CL_CONTEXT_INTEROP_USER_SYNC, CL_TRUE,
                                         0};

  cl_int contextStatus = !CL_SUCCESS;
  cl_context context =
      clCreateContext(properties, 1, &device, nullptr, nullptr, &contextStatus);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(contextStatus);

  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clCreateContextTest, SpecifyPlatform) {
  cl_context_properties properties[3] = {
      CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform),
      0};

  cl_int contextStatus = !CL_SUCCESS;
  cl_context context =
      clCreateContext(properties, 1, &device, nullptr, nullptr, &contextStatus);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(contextStatus);

  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clCreateContextTest, PropertySpecifiedMultipleTimes) {
  cl_context_properties properties[5] = {CL_CONTEXT_INTEROP_USER_SYNC, CL_TRUE,
                                         CL_CONTEXT_INTEROP_USER_SYNC, CL_FALSE,
                                         0};

  cl_int contextStatus = !CL_SUCCESS;
  cl_context context =
      clCreateContext(properties, 1, &device, nullptr, nullptr, &contextStatus);
  EXPECT_FALSE(context);
  ASSERT_EQ_ERRCODE(CL_INVALID_PROPERTY, contextStatus);
}

TEST_F(clCreateContextTest, WithGoodPlatform) {
  cl_int errcode;
  cl_context_properties properties[3];
  properties[0] = CL_CONTEXT_PLATFORM;
  properties[1] = reinterpret_cast<cl_context_properties>(platform);
  properties[2] = 0;
  cl_context context =
      clCreateContext(properties, 1, &device, nullptr, nullptr, &errcode);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clCreateContextTest, WithBadPlatform) {
  cl_int errcode;
  cl_context_properties properties[3];
  properties[0] = CL_CONTEXT_PLATFORM;
  properties[1] = 1;  // some guff value
  properties[2] = 0;
  EXPECT_FALSE(
      clCreateContext(properties, 1, &device, nullptr, nullptr, &errcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_PROPERTY, errcode);
}

// Ensure that we can create cl_context cocurrently, this test can't really
// fail except in the thread sanitizer (at the time of writing it did fail).
TEST_F(clCreateContextTest, ConcurrentCreate) {
  auto worker = [=]() {
    for (int i = 0; i < 32; i++) {
      cl_context context =
          clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
      clReleaseContext(context);
    }
  };

  const size_t threads = 4;
  UCL::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }
}

// Redmine #5115: Also support CL_CONTEXT_D3D10_DEVICE_KHR, CL_GL_CONTEXT_KHR,
// CL_EGL_DISPLAY_KHR, CL_GLX_DISPLAY_KHR, CL_WGL_HDC_KHR,
// CL_CONTEXT_ADAPTER_D3D9_KHR, CL_CONTEXT_ADAPTER_D3D9EX_KHR,
// CL_CONTEXT_ADAPTER_DXVA_KHR, CL_CONTEXT_D3D11_DEVICE_KHR
