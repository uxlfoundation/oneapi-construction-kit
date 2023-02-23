// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

TEST(muxCreateDevicesTest, Default) {
  mux_allocator_info_t allocator{mux::alloc, mux::free, nullptr};

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
    EXPECT_TRUE((mux_allocation_capabilities_coherent_host |
                 mux_allocation_capabilities_alloc_device) &
                devices[i]->info->allocation_capabilities);
    muxDestroyDevice(devices[i], allocator);
  }
}

TEST(muxCreateDevicesTest, allocation_size) {
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
    EXPECT_LT(0u, devices[i]->info->compute_units);
    muxDestroyDevice(devices[i], allocator);
  }
}

TEST(muxCreateDevicesTest, AllocatorAllocNull) {
  mux_allocator_info_t allocator = {nullptr, mux::free, nullptr};

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
  mux_allocator_info_t allocator = {mux::alloc, nullptr, nullptr};

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
  mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

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
  mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

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
  mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateDevices(0, nullptr, allocator, nullptr));
}

TEST(muxCreateDevicesTest, CorrectMembers) {
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
    EXPECT_LE(1u, devices[i]->info->native_vector_width);
    EXPECT_LE(1u, devices[i]->info->preferred_vector_width);
    muxDestroyDevice(devices[i], allocator);
  }
}
