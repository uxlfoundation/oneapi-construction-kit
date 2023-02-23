// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cstring>

#include "uur/fixtures.h"

using urPlatformGetInfoTest = uur::PlatformTest;

TEST_F(urPlatformGetInfoTest, SuccessName) {
  size_t size = 0;
  ASSERT_SUCCESS(
      urPlatformGetInfo(platform, UR_PLATFORM_INFO_NAME, 0, nullptr, &size));
  ASSERT_NE(0, size);
  std::vector<char> name(size);
  ASSERT_SUCCESS(urPlatformGetInfo(platform, UR_PLATFORM_INFO_NAME, size,
                                   name.data(), nullptr));
  ASSERT_EQ(size, std::strlen(name.data()) + 1);
}

TEST_F(urPlatformGetInfoTest, SuccessVendorName) {
  size_t size = 0;
  ASSERT_SUCCESS(urPlatformGetInfo(platform, UR_PLATFORM_INFO_VENDOR_NAME, 0,
                                   nullptr, &size));
  ASSERT_NE(0, size);
  std::vector<char> vendor_name(size);
  ASSERT_SUCCESS(urPlatformGetInfo(platform, UR_PLATFORM_INFO_VENDOR_NAME, size,
                                   vendor_name.data(), nullptr));
  ASSERT_EQ(size, std::strlen(vendor_name.data()) + 1);
}

TEST_F(urPlatformGetInfoTest, SuccessVersion) {
  size_t size = 0;
  ASSERT_SUCCESS(
      urPlatformGetInfo(platform, UR_PLATFORM_INFO_VERSION, 0, nullptr, &size));
  ASSERT_NE(0, size);
  std::vector<char> version(size);
  ASSERT_SUCCESS(urPlatformGetInfo(platform, UR_PLATFORM_INFO_VERSION, size,
                                   version.data(), nullptr));
  ASSERT_EQ(size, std::strlen(version.data()) + 1);
}

TEST_F(urPlatformGetInfoTest, SuccessExtensions) {
  size_t size = 0;
  ASSERT_SUCCESS(urPlatformGetInfo(platform, UR_PLATFORM_INFO_EXTENSIONS, 0,
                                   nullptr, &size));
  ASSERT_NE(0, size);
  std::vector<char> extensions(size);
  ASSERT_SUCCESS(urPlatformGetInfo(platform, UR_PLATFORM_INFO_EXTENSIONS, size,
                                   extensions.data(), nullptr));
  ASSERT_EQ(size, std::strlen(extensions.data()) + 1);
}

TEST_F(urPlatformGetInfoTest, SuccessProfile) {
  size_t size = 0;
  ASSERT_SUCCESS(
      urPlatformGetInfo(platform, UR_PLATFORM_INFO_PROFILE, 0, nullptr, &size));
  ASSERT_NE(0, size);
  std::vector<char> profile(size);
  ASSERT_SUCCESS(urPlatformGetInfo(platform, UR_PLATFORM_INFO_PROFILE, size,
                                   profile.data(), nullptr));
  ASSERT_EQ(size, std::strlen(profile.data()) + 1);
}

TEST_F(urPlatformGetInfoTest, InvalidNullHandlePlatform) {
  size_t size = 0;
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_HANDLE,
      urPlatformGetInfo(nullptr, UR_PLATFORM_INFO_NAME, 0, nullptr, &size));
}

TEST_F(urPlatformGetInfoTest, InvalidEnumerationPlatformInfoType) {
  size_t size = 0;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_ENUMERATION,
                   urPlatformGetInfo(platform, UR_PLATFORM_INFO_FORCE_UINT32, 0,
                                     nullptr, &size));
}
