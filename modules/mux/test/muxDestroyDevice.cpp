// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

TEST(muxDestroyDeviceTest, Default) {
  mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

  uint64_t devices_length = 0;

  ASSERT_SUCCESS(
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &devices_length));

  ASSERT_LT(0u, devices_length);

  std::vector<mux_device_info_t> device_infos(devices_length);

  ASSERT_SUCCESS(muxGetDeviceInfos(mux_device_type_all, devices_length,
                                   device_infos.data(), nullptr));

  std::vector<mux_device_t> devices(devices_length);

  ASSERT_SUCCESS(muxCreateDevices(devices_length, device_infos.data(),
                                  allocator, devices.data()));

  for (uint64_t i = 0; i < devices_length; i++) {
    muxDestroyDevice(devices[i], allocator);
  }
}
