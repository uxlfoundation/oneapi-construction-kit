// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Common.h"

class clSetEventCallbackTest : public ucl::ContextTest {
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

TEST_F(clSetEventCallbackTest, Default) {
  struct Callback {
    static void CL_CALLBACK callback(cl_event event,
                                     cl_int event_command_exec_status,
                                     void *user_data) {
      // Event callbacks trigger when their registered event status is reached
      // or surpassed, the status value being equal or lower than expected.
      if (CL_COMPLETE >= event_command_exec_status) {
        *static_cast<cl_event *>(user_data) = event;
      }
    }
  };

  cl_event hit = nullptr;
  ASSERT_SUCCESS(clSetEventCallback(event, CL_COMPLETE, Callback::callback,
                                    static_cast<void *>(&hit)));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_EQ(hit, event);
}

TEST_F(clSetEventCallbackTest, AllStatesCallback) {
  struct Callback {
    static void CL_CALLBACK callback(cl_event, cl_int event_command_exec_status,
                                     void *user_data) {
      *static_cast<cl_int *>(user_data) = event_command_exec_status;
    }
  };

  cl_int errorcode = !CL_SUCCESS;

  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &errorcode);
  EXPECT_TRUE(queue);
  ASSERT_SUCCESS(errorcode);

  cl_event markerEvent = nullptr;
  ASSERT_SUCCESS(clEnqueueMarkerWithWaitList(queue, 1, &event, &markerEvent));

  cl_int submitted = !CL_SUCCESS;
  cl_int running = !CL_SUCCESS;
  cl_int complete = !CL_SUCCESS;

  ASSERT_SUCCESS(clSetEventCallback(markerEvent, CL_SUBMITTED,
                                    Callback::callback, &submitted));
  ASSERT_SUCCESS(clSetEventCallback(markerEvent, CL_RUNNING, Callback::callback,
                                    &running));
  ASSERT_SUCCESS(clSetEventCallback(markerEvent, CL_COMPLETE,
                                    Callback::callback, &complete));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clWaitForEvents(1, &markerEvent));

  // Event callbacks trigger when their registered event status is reached or
  // surpassed, the status value being equal or lower than expected.
  EXPECT_TRUE(CL_SUBMITTED >= submitted);
  EXPECT_TRUE(CL_RUNNING >= running);
  EXPECT_TRUE(CL_COMPLETE >= complete);

  ASSERT_SUCCESS(clReleaseEvent(markerEvent));
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}

TEST_F(clSetEventCallbackTest, AllStatesCallbackOutOfOrderAddition) {
  struct Callback {
    static void CL_CALLBACK callbackSubmitted(cl_event,
                                              cl_int event_command_exec_status,
                                              void *user_data) {
      // Event callbacks trigger when their registered event status is reached
      // or surpassed, the status value being equal or lower than expected.
      if (CL_SUBMITTED >= event_command_exec_status) {
        *static_cast<bool *>(user_data) = true;
      }
    }

    static void CL_CALLBACK callbackRunning(cl_event,
                                            cl_int event_command_exec_status,
                                            void *user_data) {
      // Event callbacks trigger when their registered event status is reached
      // or surpassed, the status value being equal or lower than expected.
      if (CL_RUNNING >= event_command_exec_status) {
        *static_cast<bool *>(user_data) = true;
      }
    }

    static void CL_CALLBACK callbackComplete(cl_event,
                                             cl_int event_command_exec_status,
                                             void *user_data) {
      // Event callbacks trigger when their registered event status is reached
      // or surpassed, the status value being equal or lower than expected.
      if (CL_COMPLETE >= event_command_exec_status) {
        *static_cast<bool *>(user_data) = true;
      }
    }
  };

  cl_int errorcode = !CL_SUCCESS;

  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &errorcode);
  EXPECT_TRUE(queue);
  ASSERT_SUCCESS(errorcode);

  cl_event markerEvent = nullptr;
  ASSERT_SUCCESS(clEnqueueMarkerWithWaitList(queue, 1, &event, &markerEvent));

  bool submitted = false;
  bool running = false;
  bool complete = false;

  ASSERT_SUCCESS(clSetEventCallback(markerEvent, CL_COMPLETE,
                                    Callback::callbackComplete, &complete));
  ASSERT_SUCCESS(clSetEventCallback(markerEvent, CL_RUNNING,
                                    Callback::callbackRunning, &running));
  ASSERT_SUCCESS(clSetEventCallback(markerEvent, CL_SUBMITTED,
                                    Callback::callbackSubmitted, &submitted));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clWaitForEvents(1, &markerEvent));

  ASSERT_TRUE(submitted);
  ASSERT_TRUE(running);
  ASSERT_TRUE(complete);

  ASSERT_SUCCESS(clReleaseEvent(markerEvent));
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}

TEST_F(clSetEventCallbackTest, NegativeStateCallback) {
  struct Callback {
    static void CL_CALLBACK callback(cl_event, cl_int event_command_exec_status,
                                     void *user_data) {
      if ((CL_COMPLETE > event_command_exec_status) ||
          (CL_SUBMITTED == event_command_exec_status)) {
        *static_cast<bool *>(user_data) = true;
      }
    }
  };

  cl_int errorcode = !CL_SUCCESS;

  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &errorcode);
  EXPECT_TRUE(queue);
  ASSERT_SUCCESS(errorcode);

  cl_event markerEvent = nullptr;
  ASSERT_SUCCESS(clEnqueueMarkerWithWaitList(queue, 1, &event, &markerEvent));

  bool submitted = false;
  bool running = false;
  bool complete = false;

  ASSERT_SUCCESS(clSetEventCallback(markerEvent, CL_SUBMITTED,
                                    Callback::callback, &submitted));
  ASSERT_SUCCESS(clSetEventCallback(markerEvent, CL_RUNNING, Callback::callback,
                                    &running));
  ASSERT_SUCCESS(clSetEventCallback(markerEvent, CL_COMPLETE,
                                    Callback::callback, &complete));

  ASSERT_SUCCESS(clSetUserEventStatus(event, -1));

  ASSERT_EQ_ERRCODE(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST,
                    clWaitForEvents(1, &markerEvent));

  ASSERT_SUCCESS(clReleaseEvent(markerEvent));

  ASSERT_TRUE(submitted);
  ASSERT_TRUE(running);
  ASSERT_TRUE(complete);

  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}

TEST_F(clSetEventCallbackTest, Recursive) {
  struct Callback {
    static void CL_CALLBACK callbackDirect(cl_event event, cl_int,
                                           void *user_data) {
      ASSERT_SUCCESS(clSetEventCallback(event, CL_COMPLETE,
                                        Callback::callbackIndirect, user_data));
    }

    static void CL_CALLBACK callbackIndirect(cl_event event,
                                             cl_int event_command_exec_status,
                                             void *user_data) {
      // Event callbacks trigger when their registered event status is reached
      // or surpassed, the status value being equal or lower than expected.
      if (CL_COMPLETE >= event_command_exec_status) {
        *static_cast<cl_event *>(user_data) = event;
      }
    }
  };

  cl_event hit = nullptr;
  ASSERT_SUCCESS(clSetEventCallback(
      event, CL_COMPLETE, Callback::callbackDirect, static_cast<void *>(&hit)));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  // We assume that `clSetUserEventStatus` immediately triggers the callback.
  // That is very likely for most OpenCL implementations but not necessary.
  ASSERT_EQ(hit, event);
}

TEST_F(clSetEventCallbackTest, RecursiveForReachedStatus) {
  struct Callback {
    static void CL_CALLBACK callbackDirect(cl_event event, cl_int,
                                           void *user_data) {
      ASSERT_SUCCESS(clSetEventCallback(event, CL_COMPLETE,
                                        Callback::callbackIndirect, user_data));
    }

    static void CL_CALLBACK callbackIndirect(cl_event event,
                                             cl_int event_command_exec_status,
                                             void *user_data) {
      // Event callbacks trigger when their registered event status is reached
      // or surpassed, the status value being equal or lower than expected.
      if (CL_COMPLETE >= event_command_exec_status) {
        *static_cast<cl_event *>(user_data) = event;
      }
    }
  };

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  cl_event hit = nullptr;
  ASSERT_SUCCESS(clSetEventCallback(
      event, CL_COMPLETE, Callback::callbackDirect, static_cast<void *>(&hit)));

  // We assume that `clSetUserEventStatus` immediately triggers the callback.
  // That is very likely for most OpenCL implementations but not necessary.
  ASSERT_EQ(hit, event);
}

TEST_F(clSetEventCallbackTest, RecursiveDuringEventRelease) {
  struct Callback {
    static void CL_CALLBACK callbackDirect(cl_event event, cl_int,
                                           void *user_data) {
      ASSERT_SUCCESS(clSetEventCallback(event, CL_COMPLETE,
                                        Callback::callbackIndirect, user_data));
    }

    static void CL_CALLBACK callbackIndirect(cl_event event, cl_int,
                                             void *user_data) {
      *static_cast<cl_event *>(user_data) = event;
    }
  };

  cl_int errcode = CL_OUT_OF_RESOURCES;
  cl_event event_to_release = clCreateUserEvent(context, &errcode);
  EXPECT_TRUE(event_to_release);
  ASSERT_SUCCESS(errcode);

  cl_event hit = nullptr;
  ASSERT_SUCCESS(clSetEventCallback(event_to_release, CL_COMPLETE,
                                    Callback::callbackDirect,
                                    static_cast<void *>(&hit)));

  ASSERT_SUCCESS(clReleaseEvent(event_to_release));

  // Only comparing object addresses, not accessing deleted objects.
  ASSERT_EQ(hit, event_to_release);
}

// Redmine #5141: add negative test cases
// Redmine #5141: add test cases for callbacks registered after event has
// completed
