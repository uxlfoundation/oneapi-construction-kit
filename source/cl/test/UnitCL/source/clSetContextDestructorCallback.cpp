// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <atomic>
#include <thread>

#include "Common.h"

struct clSetContextDestructorCallbackTest : ucl::DeviceTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::DeviceTest::SetUp());
    cl_int error;
    context =
        clCreateContext(nullptr, 1, &device, contextCallback, this, &error);
    ASSERT_SUCCESS(error);
  }

  void TearDown() override {
    if (context) {
      ASSERT_SUCCESS(clReleaseContext(context));
    }
    ucl::DeviceTest::TearDown();
  }

  static void CL_CALLBACK contextCallback(const char *, const void *, size_t,
                                          void *user_data) {
    auto fixture = static_cast<clSetContextDestructorCallbackTest *>(user_data);
    fixture->contextCallbackCalled++;
    if (fixture->destructorCallbackCalled > 0) {
      fixture->destructorCallbackError = CL_INVALID_VALUE;
    }
  };

  static void CL_CALLBACK destructorCallback(cl_context context,
                                             void *user_data) {
    auto fixture = static_cast<clSetContextDestructorCallbackTest *>(user_data);
    fixture->destructorCallbackCalled++;
    if (fixture->context != context) {
      fixture->destructorCallbackError = CL_INVALID_CONTEXT;
    }
  };

  std::string reasonFor(cl_int error) {
    switch (error) {
      case CL_INVALID_VALUE:
        return "destructor callback called before context callback";
      case CL_INVALID_CONTEXT:
        return "destructor callback called with different context";
      default:
        return "unknown reason: " + std::to_string(error);
    }
  };

  cl_context context{nullptr};
  std::atomic<uint32_t> contextCallbackCalled{0};
  std::atomic<uint32_t> destructorCallbackCalled{0};
  std::atomic<cl_int> destructorCallbackError{CL_SUCCESS};
};

TEST_F(clSetContextDestructorCallbackTest, Default) {
  ASSERT_SUCCESS(
      clSetContextDestructorCallback(context, destructorCallback, this));
  ASSERT_EQ(0, destructorCallbackCalled);
  ASSERT_SUCCESS(clReleaseContext(context));
  ASSERT_EQ(1, destructorCallbackCalled);
  ASSERT_SUCCESS(destructorCallbackError) << reasonFor(destructorCallbackError);
  context = nullptr;  // has already been destroyed
}

TEST_F(clSetContextDestructorCallbackTest, Concurrent) {
  std::vector<std::thread> threads{std::thread::hardware_concurrency()};
  for (auto &thread : threads) {
    thread = std::thread([this]() {
      ASSERT_SUCCESS(clRetainContext(context));
      ASSERT_SUCCESS(
          clSetContextDestructorCallback(context, destructorCallback, this));
    });
  }
  for (auto &thread : threads) {
    thread.join();
  }
  ASSERT_EQ(0, destructorCallbackError);
  for (auto &thread : threads) {
    thread =
        std::thread([this]() { ASSERT_SUCCESS(clReleaseContext(context)); });
  }
  ASSERT_SUCCESS(clReleaseContext(context));
  for (auto &thread : threads) {
    thread.join();
  }
  ASSERT_EQ(std::thread::hardware_concurrency(), destructorCallbackCalled);
  ASSERT_SUCCESS(destructorCallbackError) << reasonFor(destructorCallbackError);
  context = nullptr;  // has already been destroyed
}

TEST_F(clSetContextDestructorCallbackTest, InvalidContext) {
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, clSetContextDestructorCallback(
                                            nullptr, destructorCallback, this));
}

TEST_F(clSetContextDestructorCallbackTest, InvalidValue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clSetContextDestructorCallback(context, nullptr, nullptr));
}
