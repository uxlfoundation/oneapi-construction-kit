// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clRetainEventTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    event = clCreateUserEvent(context, &errorcode);
    EXPECT_TRUE(event);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (event) {
      EXPECT_SUCCESS(clReleaseEvent(event));
    }
    ContextTest::TearDown();
  }

  cl_event event = nullptr;
};

TEST_F(clRetainEventTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_EVENT, clRetainEvent(nullptr));
  ASSERT_SUCCESS(clRetainEvent(event));
  ASSERT_SUCCESS(clReleaseEvent(event));
}
