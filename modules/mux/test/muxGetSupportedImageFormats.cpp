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

struct muxGetSupportedImageFormatsTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxGetSupportedImageFormatsTest);

TEST_P(muxGetSupportedImageFormatsTest, 1D) {
  if (device->info->image_support) {
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
  }
}

TEST_P(muxGetSupportedImageFormatsTest, 2D) {
  if (device->info->image_support) {
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
  }
}

TEST_P(muxGetSupportedImageFormatsTest, 3D) {
  if (device->info->image_support) {
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
  }
}

TEST_P(muxGetSupportedImageFormatsTest, MalformedDevice) {
  if (device->info->image_support) {
    const mux_image_type_e type = mux_image_type_1d;
    const mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;
    uint32_t out_count = 0;
    ASSERT_ERROR_EQ(mux_error_invalid_value,
                    muxGetSupportedImageFormats(nullptr, type, allocation_type,
                                                0, nullptr, &out_count));
  }
}

TEST_P(muxGetSupportedImageFormatsTest, MalformedImageType) {
  if (device->info->image_support) {
    const mux_image_type_e type = static_cast<mux_image_type_e>(-1);
    const mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;
    uint32_t out_count = 0;
    ASSERT_ERROR_EQ(mux_error_invalid_value,
                    muxGetSupportedImageFormats(nullptr, type, allocation_type,
                                                0, nullptr, &out_count));
  }
}

TEST_P(muxGetSupportedImageFormatsTest, MalformedAllocType) {
  if (device->info->image_support) {
    const mux_image_type_e type = mux_image_type_1d;
    const mux_allocation_type_e allocation_type =
        static_cast<mux_allocation_type_e>(-1);
    uint32_t out_count = 0;
    ASSERT_ERROR_EQ(mux_error_invalid_value,
                    muxGetSupportedImageFormats(device, type, allocation_type,
                                                0, nullptr, &out_count));
  }
}

TEST_P(muxGetSupportedImageFormatsTest, InvalidCount) {
  if (device->info->image_support) {
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

    ASSERT_ERROR_EQ(mux_error_invalid_value,
                    muxGetSupportedImageFormats(device, type, allocation_type,
                                                0, format.data(), nullptr));
  }
}
