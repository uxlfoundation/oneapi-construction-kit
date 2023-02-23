// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxDestroyImageTest : public DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroyImageTest);

TEST_P(muxDestroyImageTest, Default) {
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
    // For each of the supported image formats on this device we test creation
    // of an image
    for (uint64_t j = 0; j < out_count; j++) {
      mux_image_t outimage;
      ASSERT_SUCCESS(muxCreateImage(device, type, format[j], 16, 1, 1, 0, 0, 0,
                                    allocator, &outimage));
      muxDestroyImage(device, outimage, allocator);
    }
  }
}

TEST_P(muxDestroyImageTest, MalformedDevice) {
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
    // For each of the supported image formats on this device we test creation
    // of an image
    for (uint64_t j = 0; j < out_count; j++) {
      mux_image_t outimage;
      ASSERT_SUCCESS(muxCreateImage(device, type, format[j], 16, 1, 1, 0, 0, 0,
                                    allocator, &outimage));
      muxDestroyImage(nullptr, outimage, allocator);
      muxDestroyImage(device, nullptr, allocator);
      muxDestroyImage(device, outimage, allocator);
    }
  }
}
