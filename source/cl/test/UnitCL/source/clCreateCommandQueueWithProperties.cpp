// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

using clCreateCommandQueueWithPropertiesTest = ucl::ContextTest;

TEST_F(clCreateCommandQueueWithPropertiesTest, Default) {
  cl_int error;
  cl_command_queue command_queue =
      clCreateCommandQueueWithProperties(context, device, nullptr, &error);
  EXPECT_SUCCESS(error);
  cl_command_queue_properties properties = 0;
  size_t size = sizeof(properties);
  EXPECT_SUCCESS(clGetCommandQueueInfo(command_queue, CL_QUEUE_PROPERTIES, size,
                                       &properties, nullptr));
  EXPECT_EQ(0, properties);
  ASSERT_SUCCESS(clReleaseCommandQueue(command_queue));
}

TEST_F(clCreateCommandQueueWithPropertiesTest, DefaultProfiling) {
  cl_int error;
  std::array<cl_queue_properties, 3> properties{
      {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0}};
  cl_command_queue command_queue = clCreateCommandQueueWithProperties(
      context, device, properties.data(), &error);
  size_t size;
  EXPECT_SUCCESS(clGetCommandQueueInfo(command_queue, CL_QUEUE_PROPERTIES, 0,
                                       nullptr, &size));
  EXPECT_EQ(sizeof(cl_command_queue_properties), size);
  cl_command_queue_properties command_queue_properties;
  EXPECT_SUCCESS(clGetCommandQueueInfo(command_queue, CL_QUEUE_PROPERTIES, size,
                                       &command_queue_properties, nullptr));
  EXPECT_EQ(properties[1], command_queue_properties);
  EXPECT_SUCCESS(error);
  ASSERT_SUCCESS(clReleaseCommandQueue(command_queue));
}

TEST_F(clCreateCommandQueueWithPropertiesTest, InvalidContext) {
  cl_int error;
  cl_command_queue command_queue =
      clCreateCommandQueueWithProperties(nullptr, device, nullptr, &error);
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, error);
  ASSERT_EQ(nullptr, command_queue);
}

TEST_F(clCreateCommandQueueWithPropertiesTest, InvalidDevice) {
  cl_int error;
  cl_command_queue command_queue =
      clCreateCommandQueueWithProperties(context, nullptr, nullptr, &error);
  ASSERT_EQ_ERRCODE(CL_INVALID_DEVICE, error);
  ASSERT_EQ(nullptr, command_queue);
}

TEST_F(clCreateCommandQueueWithPropertiesTest, InvalidQueueProperties) {
  cl_int error;
  // Try with one bit that we don't support.
  std::array<cl_queue_properties, 3> properties{
      {CL_QUEUE_PROPERTIES, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, 0}};
  cl_command_queue command_queue = clCreateCommandQueueWithProperties(
      context, device, properties.data(), &error);
  ASSERT_EQ_ERRCODE(CL_INVALID_QUEUE_PROPERTIES, error);
  ASSERT_EQ(nullptr, command_queue);

  // Try with a combination of a bit we do support and one we don't.
  properties = {
      {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_ON_DEVICE, 0}};
  command_queue = clCreateCommandQueueWithProperties(context, device,
                                                     properties.data(), &error);
  ASSERT_EQ_ERRCODE(CL_INVALID_QUEUE_PROPERTIES, error);
  ASSERT_EQ(nullptr, command_queue);
}

TEST_F(clCreateCommandQueueWithPropertiesTest, InvalidValue) {
  cl_int error;
  std::array<cl_queue_properties, 3> properties{
      {CL_QUEUE_PROPERTIES, 0xFFFFFFFFFFFFFFFF, 0}};
  cl_command_queue command_queue = clCreateCommandQueueWithProperties(
      context, device, properties.data(), &error);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
  ASSERT_EQ(nullptr, command_queue);

  properties = {{CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE | 0xF0, 0}};
  command_queue = clCreateCommandQueueWithProperties(context, device,
                                                     properties.data(), &error);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
  ASSERT_EQ(nullptr, command_queue);

  properties = {{0xFFFFFFFFFFFFFFFF, 42, 0}};
  command_queue = clCreateCommandQueueWithProperties(context, device,
                                                     properties.data(), &error);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
  ASSERT_EQ(nullptr, command_queue);
}
