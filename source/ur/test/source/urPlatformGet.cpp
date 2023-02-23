// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/checks.h"

TEST(urPlatformGetTest, Success) {
  uint32_t count;
  ASSERT_SUCCESS(urPlatformGet(0, nullptr, &count));
  std::vector<ur_platform_handle_t> platforms(count);
  ASSERT_SUCCESS(urPlatformGet(count, platforms.data(), nullptr));
  for (auto platform : platforms) {
    ASSERT_NE(nullptr, platform);
  }
}

TEST(urPlatformGetTest, InvalidNumEntries) {
  uint32_t count;
  ASSERT_SUCCESS(urPlatformGet(0, nullptr, &count));
  std::vector<ur_platform_handle_t> platforms(count);
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_SIZE,
                   urPlatformGet(0, platforms.data(), nullptr));
}
