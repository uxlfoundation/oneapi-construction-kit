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

struct muxCreateImageTest : DeviceTest {
  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    if (!device->info->image_support) {
      GTEST_SKIP();
    }
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCreateImageTest);

TEST_P(muxCreateImageTest, 1D) {
  const mux_image_type_e type = mux_image_type_1d;
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;
  uint32_t out_count = 0;
  ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                             nullptr, &out_count));

  std::vector<mux_image_format_e> format(out_count);

  ASSERT_SUCCESS(muxGetSupportedImageFormats(
      device, type, allocation_type, out_count, format.data(), nullptr));
  // For each of the supported image formats on this device we test creation
  // of an image
  for (uint64_t i = 0; i < out_count; i++) {
    mux_image_t outimage;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[i], 16, 1, 1, 0, 0, 0,
                                  allocator, &outimage));
    muxDestroyImage(device, outimage, allocator);
  }
}

TEST_P(muxCreateImageTest, 2D) {
  const mux_image_type_e type = mux_image_type_2d;
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;
  uint32_t out_count = 0;
  ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                             nullptr, &out_count));

  std::vector<mux_image_format_e> format(out_count);

  ASSERT_SUCCESS(muxGetSupportedImageFormats(
      device, type, allocation_type, out_count, format.data(), nullptr));
  // For each of the supported image formats on this device we test creation
  // of an image
  for (uint64_t i = 0; i < out_count; i++) {
    mux_image_t outimage;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[i], 8, 8, 1, 0, 0, 0,
                                  allocator, &outimage));
    muxDestroyImage(device, outimage, allocator);
  }
}

TEST_P(muxCreateImageTest, 3D) {
  const mux_image_type_e type = mux_image_type_3d;
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;
  uint32_t out_count = 0;
  ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                             nullptr, &out_count));

  std::vector<mux_image_format_e> format(out_count);

  ASSERT_SUCCESS(muxGetSupportedImageFormats(
      device, type, allocation_type, out_count, format.data(), nullptr));
  // For each of the supported image formats on this device we test creation
  // of an image
  for (uint64_t i = 0; i < out_count; i++) {
    mux_image_t outimage;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[i], 4, 4, 4, 0, 0, 0,
                                  allocator, &outimage));
    muxDestroyImage(device, outimage, allocator);
  }
}

TEST_P(muxCreateImageTest, MalformedDevice) {
  const mux_image_type_e type = mux_image_type_3d;
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;
  uint32_t out_count = 0;
  ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                             nullptr, &out_count));

  std::vector<mux_image_format_e> format(out_count);

  ASSERT_SUCCESS(muxGetSupportedImageFormats(
      device, type, allocation_type, out_count, format.data(), nullptr));
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(nullptr, type, format[0], 2, 2, 2, 0, 0, 0,
                                 allocator, nullptr));
}

TEST_P(muxCreateImageTest, IncorrectImageParams1D) {
  const mux_image_type_e type = mux_image_type_1d;
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;
  uint32_t out_count = 0;
  ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                             nullptr, &out_count));

  std::vector<mux_image_format_e> format(out_count);

  ASSERT_SUCCESS(muxGetSupportedImageFormats(
      device, type, allocation_type, out_count, format.data(), nullptr));

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(device, type, format[0], 0, 1, 1, 0, 0, 0,
                                 allocator, nullptr));
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(device, type, format[0], 8, 8, 1, 0, 0, 0,
                                 allocator, nullptr));
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(device, type, format[0], 8, 1, 8, 0, 0, 0,
                                 allocator, nullptr));
}

TEST_P(muxCreateImageTest, IncorrectImageParams2D) {
  const mux_image_type_e type = mux_image_type_2d;
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;
  uint32_t out_count = 0;
  ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                             nullptr, &out_count));

  std::vector<mux_image_format_e> format(out_count);

  ASSERT_SUCCESS(muxGetSupportedImageFormats(
      device, type, allocation_type, out_count, format.data(), nullptr));

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(device, type, format[0], 0, 4, 1, 0, 0, 0,
                                 allocator, nullptr));
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(device, type, format[0], 4, 0, 1, 0, 0, 0,
                                 allocator, nullptr));
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(device, type, format[0], 4, 4, 0, 0, 0, 0,
                                 allocator, nullptr));
}

TEST_P(muxCreateImageTest, IncorrectImageParams3D) {
  const mux_image_type_e type = mux_image_type_3d;
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;
  uint32_t out_count = 0;
  ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                             nullptr, &out_count));

  std::vector<mux_image_format_e> format(out_count);

  ASSERT_SUCCESS(muxGetSupportedImageFormats(
      device, type, allocation_type, out_count, format.data(), nullptr));

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(device, type, format[0], 0, 4, 4, 0, 0, 0,
                                 allocator, nullptr));
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(device, type, format[0], 4, 0, 4, 0, 0, 0,
                                 allocator, nullptr));
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(device, type, format[0], 4, 4, 0, 0, 0, 0,
                                 allocator, nullptr));
}

TEST_P(muxCreateImageTest, NullOutParameter) {
  const mux_image_type_e type = mux_image_type_1d;
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;
  uint32_t out_count = 0;
  ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                             nullptr, &out_count));

  std::vector<mux_image_format_e> format(out_count);

  ASSERT_SUCCESS(muxGetSupportedImageFormats(
      device, type, allocation_type, out_count, format.data(), nullptr));
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateImage(device, type, format[0], 16, 1, 1, 0, 0, 0,
                                 allocator, nullptr));
}

// TODO: Implement mux_error_out_of_memory test. This is a bit fiddly to test
// for as it requires allocating too much memory for the device.
