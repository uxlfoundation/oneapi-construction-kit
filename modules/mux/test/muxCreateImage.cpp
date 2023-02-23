// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
  for (uint64_t i = 0; i < out_count; i++) {
    mux_image_t outimage;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[i], 16, 1, 1, 0, 0, 0,
                                  allocator, &outimage));
    muxDestroyImage(device, outimage, allocator);
  }
}

TEST_P(muxCreateImageTest, 2D) {
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
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateImage(nullptr, type, format[0], 2, 2, 2, 0, 0, 0,
                                 allocator, nullptr));
}

TEST_P(muxCreateImageTest, IncorrectImageParams1D) {
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
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateImage(device, type, format[0], 16, 1, 1, 0, 0, 0,
                                 allocator, nullptr));
}

// TODO: Implement mux_error_out_of_memory test. This is a bit fiddly to test
// for as it requires allocating too much memory for the device.
