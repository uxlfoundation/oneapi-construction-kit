// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

struct clCreateContextFromTypeGoodTest
    : ucl::DeviceTest,
      testing::WithParamInterface<cl_device_type> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    properties[0] = CL_CONTEXT_PLATFORM;
    properties[1] = reinterpret_cast<cl_context_properties>(platform);
    properties[2] = 0;
  }

  cl_context_properties properties[3];
};

struct clCreateContextFromTypeGoodNotFoundTest
    : ucl::DeviceTest,
      testing::WithParamInterface<cl_device_type> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    properties[0] = CL_CONTEXT_PLATFORM;
    properties[1] = reinterpret_cast<cl_context_properties>(platform);
    properties[2] = 0;
  }

  cl_context_properties properties[3];
};

struct clCreateContextFromTypeBadTest
    : ucl::DeviceTest,
      testing::WithParamInterface<cl_device_type> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    properties[0] = CL_CONTEXT_PLATFORM;
    properties[1] = reinterpret_cast<cl_context_properties>(platform);
    properties[2] = 0;
  }

  cl_context_properties properties[3];
};

TEST_P(clCreateContextFromTypeGoodTest, Default) {
  cl_int errcode;
  cl_context context = clCreateContextFromType(properties, GetParam(), nullptr,
                                               nullptr, &errcode);
  ASSERT_TRUE(context);
  EXPECT_SUCCESS(errcode);
  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_P(clCreateContextFromTypeGoodTest, DefaultCallback) {
  struct callback_data_t {
  } callback_data;
  auto callback = [](const char *, const void *, size_t, void *) {};
  cl_int errcode;
  cl_context context = clCreateContextFromType(properties, GetParam(), callback,
                                               &callback_data, &errcode);
  ASSERT_TRUE(context);
  EXPECT_SUCCESS(errcode);
  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_P(clCreateContextFromTypeGoodTest, nullptrErrorCode) {
  cl_context context = clCreateContextFromType(properties, GetParam(), nullptr,
                                               nullptr, nullptr);
  ASSERT_TRUE(context);
  EXPECT_SUCCESS(clReleaseContext(context));
}

TEST_P(clCreateContextFromTypeBadTest, Default) {
  cl_int errcode;
  EXPECT_FALSE(clCreateContextFromType(properties, GetParam(), nullptr, nullptr,
                                       &errcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_DEVICE_TYPE, errcode);
}

TEST_P(clCreateContextFromTypeBadTest, nullptrErrorCode) {
  ASSERT_FALSE(clCreateContextFromType(properties, GetParam(), nullptr, nullptr,
                                       nullptr));
}

TEST_P(clCreateContextFromTypeGoodNotFoundTest, Default) {
  cl_int errcode;
  cl_context context = clCreateContextFromType(properties, GetParam(), nullptr,
                                               nullptr, &errcode);
  if (nullptr != context) {
    EXPECT_SUCCESS(errcode);
    ASSERT_SUCCESS(clReleaseContext(context));
  } else {
    EXPECT_EQ_ERRCODE(CL_DEVICE_NOT_FOUND, errcode);
  }
}

static cl_device_type good_devices[] = {
    CL_DEVICE_TYPE_DEFAULT,
    CL_DEVICE_TYPE_ALL,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_GPU,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_ACCELERATOR,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_ACCELERATOR,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CUSTOM | CL_DEVICE_TYPE_ACCELERATOR,
    CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR |
        CL_DEVICE_TYPE_CUSTOM,
    CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU |
        CL_DEVICE_TYPE_ACCELERATOR | CL_DEVICE_TYPE_CUSTOM};

static cl_device_type good_devices_not_found[] = {
    CL_DEVICE_TYPE_DEFAULT, CL_DEVICE_TYPE_ALL,         CL_DEVICE_TYPE_CPU,
    CL_DEVICE_TYPE_GPU,     CL_DEVICE_TYPE_ACCELERATOR, CL_DEVICE_TYPE_CUSTOM};

static cl_device_type bad_devices[] = {
    (cl_device_type)(~(CL_DEVICE_TYPE_DEFAULT | CL_DEVICE_TYPE_CPU |
                       CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR |
                       CL_DEVICE_TYPE_CUSTOM)),
    0u};

INSTANTIATE_TEST_CASE_P(clCreateContextFromType,
                        clCreateContextFromTypeGoodTest,
                        ::testing::ValuesIn(good_devices));
INSTANTIATE_TEST_CASE_P(clCreateContextFromType,
                        clCreateContextFromTypeGoodNotFoundTest,
                        ::testing::ValuesIn(good_devices_not_found));
INSTANTIATE_TEST_CASE_P(clCreateContextFromType, clCreateContextFromTypeBadTest,
                        ::testing::ValuesIn(bad_devices));
