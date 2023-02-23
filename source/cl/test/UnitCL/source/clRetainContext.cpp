// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

using clRetainContextTest = ucl::ContextTest;

TEST_F(clRetainContextTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_CONTEXT, clRetainContext(nullptr));
  ASSERT_SUCCESS(clRetainContext(context));
  ASSERT_SUCCESS(clReleaseContext(context));
}
