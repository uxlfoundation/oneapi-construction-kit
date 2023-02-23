// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UNITCL_EVENT_WAIT_LIST_H
#define UNITCL_EVENT_WAIT_LIST_H

#include <CL/cl.h>

/// @brief Base class for tests requiring event list testing.
class TestWithEventWaitList {
 protected:
  /// @brief Method performing the actual API calls when testing for event list
  /// error codes.
  ///
  /// @param errorcode The errorcode that should be expected from the API call
  /// given the event list parameters.
  /// @param num_events Number of events in the event list.
  /// @param events Event list.
  /// @param event Return event.
  ///
  /// @gotchas For calls that can be blocking the API call used in this function
  /// should be marked as blocking for the tests to work properly.
  virtual void EventWaitListAPICall(cl_int errorcode, cl_uint num_events,
                                    const cl_event *events,
                                    cl_event *event) = 0;
};

/// @brief Generate the tests for an event wait list for a given test class.
///
/// The test class must implement the TestWithEventWaitList interface and store
/// its context as a member named `context`.
///
/// @param TEST_NAME Name of the test class to which the tests should be added.
#define GENERATE_EVENT_WAIT_LIST_TESTS(TEST_NAME)                          \
  TEST_F(TEST_NAME, EventWaitListNullSize1) {                              \
    EventWaitListAPICall(CL_INVALID_EVENT_WAIT_LIST, 1, nullptr, nullptr); \
  }                                                                        \
                                                                           \
  TEST_F(TEST_NAME, EventWaitListNonNullSize0) {                           \
    cl_event event;                                                        \
    EventWaitListAPICall(CL_INVALID_EVENT_WAIT_LIST, 0, &event, nullptr);  \
  }                                                                        \
                                                                           \
  TEST_F(TEST_NAME, EventWaitListNullEvent) {                              \
    cl_event event = nullptr;                                              \
    EventWaitListAPICall(CL_INVALID_EVENT_WAIT_LIST, 1, &event, nullptr);  \
  }                                                                        \
                                                                           \
  TEST_F(TEST_NAME, EventWaitListReturnEvent) {                            \
    cl_int errcode;                                                        \
    cl_event event = clCreateUserEvent(context, &errcode);                 \
    ASSERT_TRUE(event);                                                    \
    ASSERT_SUCCESS(errcode);                                               \
                                                                           \
    ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));              \
                                                                           \
    EventWaitListAPICall(CL_INVALID_EVENT_WAIT_LIST, 1, &event, &event);   \
                                                                           \
    EXPECT_SUCCESS(clReleaseEvent(event));                                 \
  }                                                                        \
                                                                           \
  TEST_F(TEST_NAME, EventWaitListContextMismatch) {                        \
    cl_int errcode;                                                        \
    cl_context otherContext =                                              \
        clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errcode);  \
    EXPECT_TRUE(otherContext);                                             \
    ASSERT_SUCCESS(errcode);                                               \
                                                                           \
    cl_event user_event = clCreateUserEvent(otherContext, &errcode);       \
    ASSERT_TRUE(user_event);                                               \
    ASSERT_SUCCESS(errcode);                                               \
                                                                           \
    ASSERT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));         \
                                                                           \
    EventWaitListAPICall(CL_INVALID_CONTEXT, 1, &user_event, nullptr);     \
                                                                           \
    EXPECT_SUCCESS(clReleaseEvent(user_event));                            \
    EXPECT_SUCCESS(clReleaseContext(otherContext));                        \
  }

/// @brief Generate the tests for an event wait list including the test for
/// blocking calls.
///
/// The test class must implement the TestWithEventWaitList interface and store
/// its context as a member named `context`.
///
/// @gotchas For the blocking specific test to work, the API call must be
/// defined as blocking.
///
/// @param TEST_NAME Name of the test class to which the tests should be added.
#define GENERATE_EVENT_WAIT_LIST_TESTS_BLOCKING(TEST_NAME)                \
  GENERATE_EVENT_WAIT_LIST_TESTS(TEST_NAME)                               \
                                                                          \
  TEST_F(TEST_NAME, EventWaitListBlockingFailedEvent) {                   \
    cl_int errcode;                                                       \
    cl_event user_event = clCreateUserEvent(context, &errcode);           \
    ASSERT_TRUE(user_event);                                              \
    ASSERT_SUCCESS(errcode);                                              \
                                                                          \
    ASSERT_SUCCESS(clSetUserEventStatus(user_event, -1));                 \
                                                                          \
    EventWaitListAPICall(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST, 1, \
                         &user_event, nullptr);                           \
                                                                          \
    EXPECT_SUCCESS(clReleaseEvent(user_event));                           \
  }

#endif
