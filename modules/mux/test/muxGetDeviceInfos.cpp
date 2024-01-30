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

#include "common.h"

TEST(muxGetDeviceInfos, Default) {
  uint64_t devices_length = 0;

  ASSERT_SUCCESS(
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &devices_length));

  ASSERT_LT(0u, devices_length);

  std::vector<mux_device_info_t> device_infos(devices_length);

  ASSERT_SUCCESS(muxGetDeviceInfos(mux_device_type_all, devices_length,
                                   device_infos.data(), nullptr));
}

TEST(muxGetDeviceInfos, ZeroLength) {
  uint64_t devices_length = 0;

  ASSERT_SUCCESS(
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &devices_length));

  ASSERT_LT(0u, devices_length);

  std::vector<mux_device_info_t> device_infos(devices_length);

  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetDeviceInfos(mux_device_type_all, 0, device_infos.data(), nullptr));
}

TEST(muxGetDeviceInfos, NullInfos) {
  uint64_t devices_length = 0;

  ASSERT_SUCCESS(
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &devices_length));

  ASSERT_LT(0u, devices_length);

  const std::vector<mux_device_info_t> device_infos(devices_length);

  ASSERT_ERROR_EQ(
      mux_error_null_out_parameter,
      muxGetDeviceInfos(mux_device_type_all, devices_length, nullptr, nullptr));
}

TEST(muxGetDeviceInfos, AllNull) {
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxGetDeviceInfos(mux_device_type_all, 0, nullptr, nullptr));
}

TEST(muxGetDeviceInfos, NotCompiler) {
  uint64_t devices_length = 0;
  ASSERT_SUCCESS(
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &devices_length));
  if (devices_length > 0) {
    std::vector<mux_device_info_t> device_infos(devices_length);
    ASSERT_SUCCESS(muxGetDeviceInfos(mux_device_type_all, devices_length,
                                     device_infos.data(), nullptr));
  }
}
