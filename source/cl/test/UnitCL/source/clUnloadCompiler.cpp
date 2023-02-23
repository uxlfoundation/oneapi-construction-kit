// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

using clUnloadCompilerTest = testing::Test;

// This test is disabled because the function is deprecated in OpenCL 1.2, and
// is fundamentally incompatible with an ICD (due to not taking a platform
// parameter).  See clUnloadPlatformCompiler instead.
TEST_F(clUnloadCompilerTest, DISABLED_Default) {
  ASSERT_SUCCESS(clUnloadCompiler());
}
