// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxGetSupportedImageFormatsTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxGetSupportedImageFormatsTest);

TEST_P(muxGetSupportedImageFormatsTest, 1D) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_1d;
    mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;
    uint32_t out_count = 0;
    ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                               nullptr, &out_count));

    std::vector<mux_image_format_e> format(out_count);

    ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type,
                                               out_count, &format[0], nullptr));
  }
}

TEST_P(muxGetSupportedImageFormatsTest, 2D) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_2d;
    mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;
    uint32_t out_count = 0;
    ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                               nullptr, &out_count));

    std::vector<mux_image_format_e> format(out_count);

    ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type,
                                               out_count, &format[0], nullptr));
  }
}

TEST_P(muxGetSupportedImageFormatsTest, 3D) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_3d;
    mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;
    uint32_t out_count = 0;
    ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type, 0,
                                               nullptr, &out_count));

    std::vector<mux_image_format_e> format(out_count);

    ASSERT_SUCCESS(muxGetSupportedImageFormats(device, type, allocation_type,
                                               out_count, &format[0], nullptr));
  }
}

TEST_P(muxGetSupportedImageFormatsTest, MalformedDevice) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_1d;
    mux_allocation_type_e allocation_type =
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
    mux_image_type_e type = static_cast<mux_image_type_e>(-1);
    mux_allocation_type_e allocation_type =
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
    mux_image_type_e type = mux_image_type_1d;
    mux_allocation_type_e allocation_type =
        static_cast<mux_allocation_type_e>(-1);
    uint32_t out_count = 0;
    ASSERT_ERROR_EQ(mux_error_invalid_value,
                    muxGetSupportedImageFormats(device, type, allocation_type,
                                                0, nullptr, &out_count));
  }
}

TEST_P(muxGetSupportedImageFormatsTest, InvalidCount) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_1d;
    mux_allocation_type_e allocation_type =
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
                                                0, &format[0], nullptr));
  }
}
