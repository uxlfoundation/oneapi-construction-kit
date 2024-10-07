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

using clGetContextInfoTest = ucl::ContextTest;

// Redmine #5115: Additional tests required for directx

TEST_F(clGetContextInfoTest, BadContext) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clGetContextInfo(nullptr, CL_CONTEXT_PROPERTIES, 0, nullptr, nullptr));
}

TEST_F(clGetContextInfoTest, NullRetPointers) {
  // spec seems to say that this should silently fail
  ASSERT_SUCCESS(clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, 0,
                                  nullptr, nullptr));
}

TEST_F(clGetContextInfoTest, ContextRefCountSizeRet) {
  size_t size;
  ASSERT_SUCCESS(
      clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
}

TEST_F(clGetContextInfoTest, ContextRefCountDefault) {
  size_t size;
  ASSERT_SUCCESS(
      clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, 0, nullptr, &size));
  cl_uint ref_count;
  ASSERT_SUCCESS(clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, size,
                                  &ref_count, nullptr));
}

TEST_F(clGetContextInfoTest, BadContextRefCountSize) {
  cl_uint ref_count;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, 0,
                                     &ref_count, nullptr));
}

TEST_F(clGetContextInfoTest, ContextNumDevicesSizeRet) {
  size_t size;
  ASSERT_SUCCESS(
      clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
}

TEST_F(clGetContextInfoTest, ContextNumDevicesDefault) {
  size_t size;
  ASSERT_SUCCESS(
      clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, 0, nullptr, &size));
  cl_uint numDevices;
  ASSERT_SUCCESS(clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, size,
                                  &numDevices, nullptr));
  ASSERT_EQ(numDevices, 1);
}

TEST_F(clGetContextInfoTest, BadContextNumDevicesSize) {
  cl_uint numDevices;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, 0,
                                     &numDevices, nullptr));
}

TEST_F(clGetContextInfoTest, ContextDevicesDefault) {
  size_t size;
  ASSERT_SUCCESS(
      clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_id), size);

  UCL::Buffer<cl_device_id> devices(size / sizeof(cl_device_id));

  EXPECT_SUCCESS(clGetContextInfo(context, CL_CONTEXT_DEVICES, size,
                                  static_cast<void *>(devices), nullptr));

  bool foundAllDevices = true;
  for (unsigned int i = 0; i < devices.size(); i++) {
    bool foundThisDevice = false;
    for (unsigned int k = 0; k < 1; k++) {
      if (devices[i] == device) {
        foundThisDevice = true;
      }
    }

    foundAllDevices &= foundThisDevice;
  }

  ASSERT_TRUE(foundAllDevices);
}

TEST_F(clGetContextInfoTest, BadContextDeviceSize) {
  cl_device_id device = nullptr;
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetContextInfo(context, CL_CONTEXT_DEVICES, 0,
                                     static_cast<void *>(&device), nullptr));
  ASSERT_FALSE(device);
}

TEST_F(clGetContextInfoTest, GetProperties) {
  enum { NUM_PROPERTIES = 3 };

  cl_int err = !CL_SUCCESS;

  cl_context_properties properties[NUM_PROPERTIES];
  properties[0] = CL_CONTEXT_PLATFORM;
  properties[1] = reinterpret_cast<cl_context_properties>(platform);
  properties[2] = 0;

  cl_context context =
      clCreateContext(properties, 1, &device, nullptr, nullptr, &err);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(err);

  size_t size;
  ASSERT_SUCCESS(
      clGetContextInfo(context, CL_CONTEXT_PROPERTIES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_context_properties) * NUM_PROPERTIES, size);

  cl_context_properties other_properties[NUM_PROPERTIES];

  EXPECT_SUCCESS(clGetContextInfo(context, CL_CONTEXT_PROPERTIES, size,
                                  other_properties, nullptr));
  const cl_context_properties contextPlatform = CL_CONTEXT_PLATFORM;
  EXPECT_EQ(contextPlatform, other_properties[0]);
  EXPECT_EQ(platform, reinterpret_cast<cl_platform_id>(other_properties[1]));
  EXPECT_EQ(0, other_properties[2]);

  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clGetContextInfoTest, GetPropertiesFromType) {
  enum { NUM_PROPERTIES = 3 };

  cl_int err = !CL_SUCCESS;

  cl_context_properties properties[NUM_PROPERTIES];
  properties[0] = CL_CONTEXT_PLATFORM;
  properties[1] = reinterpret_cast<cl_context_properties>(platform);
  properties[2] = 0;

  cl_context context = clCreateContextFromType(
      properties, CL_DEVICE_TYPE_DEFAULT, nullptr, nullptr, &err);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(err);

  size_t size;
  ASSERT_SUCCESS(
      clGetContextInfo(context, CL_CONTEXT_PROPERTIES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_context_properties) * NUM_PROPERTIES, size);

  cl_context_properties other_properties[NUM_PROPERTIES];

  EXPECT_SUCCESS(clGetContextInfo(context, CL_CONTEXT_PROPERTIES, size,
                                  other_properties, nullptr));

  const cl_context_properties contextPlatform = CL_CONTEXT_PLATFORM;
  EXPECT_EQ(contextPlatform, other_properties[0]);
  EXPECT_EQ(reinterpret_cast<cl_context_properties>(platform),
            other_properties[1]);
  EXPECT_EQ(0, other_properties[2]);

  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clGetContextInfoTest, GetNullProperties) {
  size_t size;
  ASSERT_SUCCESS(
      clGetContextInfo(context, CL_CONTEXT_PROPERTIES, 0, nullptr, &size));
  ASSERT_EQ(0u, size);
}
