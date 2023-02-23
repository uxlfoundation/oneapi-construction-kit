// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

using clUnloadPlatformCompilerTest = ucl::PlatformTest;

TEST_F(clUnloadPlatformCompilerTest, Default) {
  ASSERT_SUCCESS(clUnloadPlatformCompiler(platform));
}

TEST_F(clUnloadPlatformCompilerTest, NullPlatform) {
  ASSERT_EQ_ERRCODE(CL_INVALID_PLATFORM, clUnloadPlatformCompiler(nullptr));
}
