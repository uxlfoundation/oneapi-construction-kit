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

#include "uur/fixtures.h"

using urDeviceGetTest = uur::PlatformTest;

TEST_F(urDeviceGetTest, Success) {
  uint32_t count;
  ASSERT_SUCCESS(urDeviceGet(platform, UR_DEVICE_TYPE_ALL, 0, nullptr, &count));
  std::vector<ur_device_handle_t> devices(count);
  ASSERT_SUCCESS(urDeviceGet(platform, UR_DEVICE_TYPE_ALL, count,
                             devices.data(), nullptr));
  for (auto device : devices) {
    ASSERT_NE(nullptr, device);
  }
}

TEST_F(urDeviceGetTest, SuccessSubsetOfDevices) {
  uint32_t count;
  ASSERT_SUCCESS(urDeviceGet(platform, UR_DEVICE_TYPE_ALL, 0, nullptr, &count));
  if (count < 2) {
    GTEST_SKIP();
  }
  std::vector<ur_device_handle_t> devices(count - 1);
  ASSERT_SUCCESS(urDeviceGet(platform, UR_DEVICE_TYPE_ALL, count - 1,
                             devices.data(), nullptr));
  for (auto device : devices) {
    ASSERT_NE(nullptr, device);
  }
}

TEST_F(urDeviceGetTest, InvalidNullHandlePlatform) {
  uint32_t count;
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_HANDLE,
      urDeviceGet(nullptr, UR_DEVICE_TYPE_ALL, 0, nullptr, &count));
}

TEST_F(urDeviceGetTest, InvalidEnumerationDevicesType) {
  uint32_t count;
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_ENUMERATION,
      urDeviceGet(platform, UR_DEVICE_TYPE_FORCE_UINT32, 0, nullptr, &count));
}

TEST_F(urDeviceGetTest, InvalidNumEntries) {
  uint32_t count;
  ASSERT_SUCCESS(urDeviceGet(platform, UR_DEVICE_TYPE_ALL, 0, nullptr, &count));
  std::vector<ur_device_handle_t> devices(count);
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_SIZE,
      urDeviceGet(platform, UR_DEVICE_TYPE_ALL, 0, devices.data(), nullptr));
}
