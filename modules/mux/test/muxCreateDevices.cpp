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

TEST(muxCreateDevicesTest, Default) {
  const mux_allocator_info_t allocator{mux::alloc, mux::free, nullptr};

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
    EXPECT_EQ(device_infos[i]->id, devices[i]->id);
    muxDestroyDevice(devices[i], allocator);
  }
}

TEST(muxCreateDevicesTest, allocation_capabilities) {
  const mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

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
    EXPECT_TRUE((mux_allocation_capabilities_coherent_host |
                 mux_allocation_capabilities_alloc_device) &
                devices[i]->info->allocation_capabilities);
    muxDestroyDevice(devices[i], allocator);
  }
}

TEST(muxCreateDevicesTest, allocation_size) {
  const mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

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
    if (devices[i]->info->memory_size > 0u) {
      // If the device has memory then we need to be able to allocate some.
      EXPECT_LT(0u, devices[i]->info->allocation_size);
    }

    // Can't allocate more memory than we have.
    EXPECT_LE(devices[i]->info->allocation_size, devices[i]->info->memory_size);
    muxDestroyDevice(devices[i], allocator);
  }
}

TEST(muxCreateDevicesTest, compute_units) {
  const mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

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
    EXPECT_LT(0u, devices[i]->info->compute_units);
    muxDestroyDevice(devices[i], allocator);
  }
}

TEST(muxCreateDevicesTest, AllocatorAllocNull) {
  const mux_allocator_info_t allocator = {nullptr, mux::free, nullptr};

  uint64_t devices_length = 0;

  ASSERT_SUCCESS(
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &devices_length));

  ASSERT_LT(0u, devices_length);

  std::vector<mux_device_info_t> device_infos(devices_length);

  ASSERT_SUCCESS(muxGetDeviceInfos(mux_device_type_all, devices_length,
                                   device_infos.data(), nullptr));

  std::vector<mux_device_t> devices(devices_length);

  ASSERT_ERROR_EQ(mux_error_null_allocator_callback,
                  muxCreateDevices(devices_length, device_infos.data(),
                                   allocator, devices.data()));
}

TEST(muxCreateDevicesTest, AllocatorFreeNull) {
  const mux_allocator_info_t allocator = {mux::alloc, nullptr, nullptr};

  uint64_t devices_length = 0;

  ASSERT_SUCCESS(
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &devices_length));

  ASSERT_LT(0u, devices_length);

  std::vector<mux_device_info_t> device_infos(devices_length);

  ASSERT_SUCCESS(muxGetDeviceInfos(mux_device_type_all, devices_length,
                                   device_infos.data(), nullptr));

  std::vector<mux_device_t> devices(devices_length);

  ASSERT_ERROR_EQ(mux_error_null_allocator_callback,
                  muxCreateDevices(devices_length, device_infos.data(),
                                   allocator, devices.data()));
}

TEST(muxCreateDevicesTest, DevicesLengthZero) {
  const mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

  uint64_t devices_length = 0;

  ASSERT_SUCCESS(
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &devices_length));

  ASSERT_LT(0u, devices_length);

  std::vector<mux_device_info_t> device_infos(devices_length);

  ASSERT_SUCCESS(muxGetDeviceInfos(mux_device_type_all, devices_length,
                                   device_infos.data(), nullptr));

  std::vector<mux_device_t> devices(devices_length);

  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCreateDevices(0, device_infos.data(), allocator, devices.data()));
}

TEST(muxCreateDevicesTest, OutDevicesNull) {
  const mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

  uint64_t devices_length = 0;

  ASSERT_SUCCESS(
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &devices_length));

  ASSERT_LT(0u, devices_length);

  std::vector<mux_device_info_t> device_infos(devices_length);

  ASSERT_SUCCESS(muxGetDeviceInfos(mux_device_type_all, devices_length,
                                   device_infos.data(), nullptr));

  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateDevices(devices_length, device_infos.data(),
                                   allocator, nullptr));
}

TEST(muxCreateDevicesTest, AllNull) {
  const mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateDevices(0, nullptr, allocator, nullptr));
}

TEST(muxCreateDevicesTest, CorrectMembers) {
  const mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

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
    EXPECT_LE(1u, devices[i]->info->native_vector_width);
    EXPECT_LE(1u, devices[i]->info->preferred_vector_width);
    muxDestroyDevice(devices[i], allocator);
  }
}
