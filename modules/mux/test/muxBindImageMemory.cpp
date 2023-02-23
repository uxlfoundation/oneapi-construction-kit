// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <gtest/gtest.h>
#include <mux/mux.h>
#include <mux/utils/helpers.h>

#include "common.h"

enum { MEMORY_SIZE = 512 };

struct muxBindImageMemoryTest : DeviceTest {
  mux_memory_t memory = nullptr;

  mux_result_t allocateMemory(uint32_t supported_heaps) {
    const mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;

    uint32_t heap = mux::findFirstSupportedHeap(supported_heaps);

    return muxAllocateMemory(device, MEMORY_SIZE, heap,
                             mux_memory_property_host_visible, allocation_type,
                             0, allocator, &memory);
  }

  void TearDown() override {
    if (device && memory) {
      muxFreeMemory(device, memory, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxBindImageMemoryTest);

TEST_P(muxBindImageMemoryTest, 1D) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_1d;
    const mux_allocation_type_e allocation_type =
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
    mux_image_t image;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[0], 16, 1, 1, 0, 0, 0,
                                  allocator, &image));
    ASSERT_SUCCESS(allocateMemory(image->memory_requirements.supported_heaps));

    ASSERT_SUCCESS(muxBindImageMemory(device, memory, image, 0));

    muxDestroyImage(device, image, allocator);
  }
}

TEST_P(muxBindImageMemoryTest, 2D) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_2d;
    const mux_allocation_type_e allocation_type =
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
    mux_image_t image;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[0], 12, 12, 1, 0, 0, 0,
                                  allocator, &image));
    ASSERT_SUCCESS(allocateMemory(image->memory_requirements.supported_heaps));

    ASSERT_SUCCESS(muxBindImageMemory(device, memory, image, 0));

    muxDestroyImage(device, image, allocator);
  }
}

TEST_P(muxBindImageMemoryTest, 3D) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_3d;
    const mux_allocation_type_e allocation_type =
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
    mux_image_t image;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[0], 8, 8, 8, 0, 0, 0,
                                  allocator, &image));
    ASSERT_SUCCESS(allocateMemory(image->memory_requirements.supported_heaps));

    ASSERT_SUCCESS(muxBindImageMemory(device, memory, image, 0));

    muxDestroyImage(device, image, allocator);
  }
}

TEST_P(muxBindImageMemoryTest, InvalidDevice) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_3d;
    const mux_allocation_type_e allocation_type =
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
    mux_image_t image;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[0], 16, 16, 16, 0, 0, 0,
                                  allocator, &image));
    ASSERT_SUCCESS(allocateMemory(image->memory_requirements.supported_heaps));

    EXPECT_EQ(mux_error_invalid_value,
              muxBindImageMemory(nullptr, memory, image, 0));

    muxDestroyImage(device, image, allocator);
  }
}

TEST_P(muxBindImageMemoryTest, InvalidMemory) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_3d;
    const mux_allocation_type_e allocation_type =
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
    mux_image_t image;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[0], 16, 16, 16, 0, 0, 0,
                                  allocator, &image));
    ASSERT_SUCCESS(allocateMemory(image->memory_requirements.supported_heaps));

    EXPECT_EQ(mux_error_invalid_value,
              muxBindImageMemory(device, nullptr, image, 0));

    muxDestroyImage(device, image, allocator);
  }
}

TEST_P(muxBindImageMemoryTest, InvalidImage) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_3d;
    const mux_allocation_type_e allocation_type =
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
    mux_image_t image;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[0], 16, 16, 16, 0, 0, 0,
                                  allocator, &image));
    ASSERT_SUCCESS(allocateMemory(image->memory_requirements.supported_heaps));

    EXPECT_EQ(mux_error_invalid_value,
              muxBindImageMemory(device, memory, nullptr, 0));

    muxDestroyImage(device, image, allocator);
  }
}

TEST_P(muxBindImageMemoryTest, InvalidImageSize) {
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_3d;
    const mux_allocation_type_e allocation_type =
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
    mux_image_t image;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[0], 64, 64, 64, 0, 0, 0,
                                  allocator, &image));
    ASSERT_SUCCESS(allocateMemory(image->memory_requirements.supported_heaps));

    EXPECT_EQ(mux_error_invalid_value,
              muxBindImageMemory(device, memory, image, 0));

    muxDestroyImage(device, image, allocator);
  }
}

TEST_P(muxBindImageMemoryTest, InvalidOffset) {
  // We test for each of the devices where length is number of devices
  if (device->info->image_support) {
    mux_image_type_e type = mux_image_type_3d;
    const mux_allocation_type_e allocation_type =
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
    mux_image_t image;
    ASSERT_SUCCESS(muxCreateImage(device, type, format[0], 4, 4, 4, 0, 0, 0,
                                  allocator, &image));
    ASSERT_SUCCESS(allocateMemory(image->memory_requirements.supported_heaps));
    EXPECT_EQ(mux_error_invalid_value,
              muxBindImageMemory(device, memory, image, memory->size));

    muxDestroyImage(device, image, allocator);
  }
}
