// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Common.h"

struct clGetDeviceIDsGoodTest : ucl::PlatformTest,
                                testing::WithParamInterface<cl_device_type> {};
using clGetDeviceIDsBadTest = clGetDeviceIDsGoodTest;

TEST_P(clGetDeviceIDsGoodTest, DefaultGood) {
  cl_uint num_devices;
  ASSERT_SUCCESS(
      clGetDeviceIDs(platform, GetParam(), 0, nullptr, &num_devices));

  UCL::Buffer<cl_device_id> devices(num_devices);

  ASSERT_SUCCESS(
      clGetDeviceIDs(platform, GetParam(), num_devices, devices, nullptr));

  for (cl_uint i = 0; i < num_devices; i++) {
    ASSERT_TRUE(devices[i]);
    ASSERT_SUCCESS(clReleaseDevice(devices[i]));
  }
}

TEST_F(clGetDeviceIDsGoodTest, InvalidDeviceType) {
  cl_uint num_devices;
  ASSERT_EQ_ERRCODE(CL_INVALID_DEVICE_TYPE,
                    clGetDeviceIDs(platform, 0, 0, nullptr, &num_devices));
}

TEST_P(clGetDeviceIDsBadTest, DefaultBad) {
  cl_uint num_devices;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_DEVICE_TYPE,
      clGetDeviceIDs(platform, GetParam(), 0, nullptr, &num_devices));
}

TEST(clGetDeviceIDs, NullPlatform) {
  cl_uint num_devices;
  const cl_int error =
      clGetDeviceIDs(nullptr, CL_DEVICE_TYPE_DEFAULT, 0, nullptr, &num_devices);

  // Unfortunately, platform being nullptr is implementation defined, so we have
  // to
  // allow either case!
  ASSERT_TRUE((CL_INVALID_PLATFORM == error) || (CL_SUCCESS == error));

  if (CL_SUCCESS == error) {
    ASSERT_GT(num_devices, 0u);
  }
}

TEST_F(clGetDeviceIDsGoodTest, EntriesWithoutDevices) {
  cl_uint num_devices;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, nullptr,
                                   &num_devices));
}

TEST_F(clGetDeviceIDsGoodTest, DevicesWithoutEntries) {
  cl_device_id device;
  cl_uint num_devices;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 0, &device,
                                   &num_devices));
}

TEST_F(clGetDeviceIDsGoodTest, DevicesAndNumDevicesNull) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 0, nullptr, nullptr));
}

static cl_device_type good_devices[] = {
    CL_DEVICE_TYPE_DEFAULT,
    CL_DEVICE_TYPE_ALL,
    CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR |
        CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_GPU,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_ACCELERATOR,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_ACCELERATOR,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_ACCELERATOR | CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU |
        CL_DEVICE_TYPE_ACCELERATOR,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU |
        CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_ACCELERATOR |
        CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR |
        CL_DEVICE_TYPE_CUSTOM};

static cl_device_type bad_devices[] = {static_cast<cl_device_type>(
    ~(CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU |
      CL_DEVICE_TYPE_ACCELERATOR | CL_DEVICE_TYPE_CUSTOM))};

INSTANTIATE_TEST_CASE_P(clGetDeviceIDs, clGetDeviceIDsGoodTest,
                        ::testing::ValuesIn(good_devices));
INSTANTIATE_TEST_CASE_P(clGetDeviceIDs, clGetDeviceIDsBadTest,
                        ::testing::ValuesIn(bad_devices));
