// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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

  std::vector<mux_device_info_t> device_infos(devices_length);

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
