// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

TEST(clGetExtensionFunctionAddressTest, PassAnyString) {
  ASSERT_FALSE(clGetExtensionFunctionAddress("anything"));
}

TEST(clGetExtensionFunctionAddressTest, PassNullString) {
  ASSERT_FALSE(clGetExtensionFunctionAddress(nullptr));
}
