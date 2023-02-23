// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

using clGetExtensionFunctionAddressForPlatformTest = ucl::PlatformTest;

TEST_F(clGetExtensionFunctionAddressForPlatformTest, PassAnyString) {
  ASSERT_FALSE(clGetExtensionFunctionAddressForPlatform(platform, "anything"));
}

TEST_F(clGetExtensionFunctionAddressForPlatformTest, PassNullString) {
  ASSERT_FALSE(clGetExtensionFunctionAddressForPlatform(platform, nullptr));
}

TEST_F(clGetExtensionFunctionAddressForPlatformTest, InvalidPlatform) {
  ASSERT_FALSE(clGetExtensionFunctionAddressForPlatform(nullptr, "anything"));
}
