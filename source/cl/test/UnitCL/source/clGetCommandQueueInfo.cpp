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

class clGetCommandQueueInfoTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES,
                                   sizeof(deviceSupportedProperties),
                                   &deviceSupportedProperties, nullptr));
    cl_int errcode;
#if defined(CL_VERSION_3_0)
    std::array<cl_command_queue_properties, 3> properties = {
        CL_QUEUE_PROPERTIES, deviceSupportedProperties, 0};
    queue = clCreateCommandQueueWithProperties(context, device,
                                               properties.data(), &errcode);
#else
    queue = clCreateCommandQueue(context, device, deviceSupportedProperties,
                                 &errcode);
#endif
    EXPECT_TRUE(queue);
    ASSERT_SUCCESS(errcode);
  }

  void TearDown() override {
    if (queue) {
      ASSERT_SUCCESS(clReleaseCommandQueue(queue));
    }
    ContextTest::TearDown();
  }

  cl_command_queue_properties deviceSupportedProperties;
  cl_command_queue queue;
};

TEST_F(clGetCommandQueueInfoTest, InvalidCommandQueue) {
  size_t size;
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE,
                    clGetCommandQueueInfo(nullptr, 0, 0, nullptr, &size));
}

TEST_F(clGetCommandQueueInfoTest, InvalidParamName) {
  size_t size;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clGetCommandQueueInfo(queue, CL_SUCCESS,
                                                            0, nullptr, &size));
}

TEST_F(clGetCommandQueueInfoTest, QueueContext) {
  size_t size;
  ASSERT_SUCCESS(
      clGetCommandQueueInfo(queue, CL_QUEUE_CONTEXT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_context), size);
  cl_context queueContext = nullptr;
  ASSERT_SUCCESS(clGetCommandQueueInfo(queue, CL_QUEUE_CONTEXT, size,
                                       static_cast<void *>(&queueContext),
                                       nullptr));
  ASSERT_EQ(context, queueContext);
}

TEST_F(clGetCommandQueueInfoTest, QueueDevice) {
  size_t size;
  ASSERT_SUCCESS(
      clGetCommandQueueInfo(queue, CL_QUEUE_DEVICE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_id), size);
  cl_device_id queueDevice = nullptr;
  ASSERT_SUCCESS(clGetCommandQueueInfo(queue, CL_QUEUE_DEVICE, size,
                                       static_cast<void *>(&queueDevice),
                                       nullptr));
  ASSERT_EQ(device, queueDevice);
}

TEST_F(clGetCommandQueueInfoTest, QueueReferenceCount) {
  size_t size;
  ASSERT_SUCCESS(clGetCommandQueueInfo(queue, CL_QUEUE_REFERENCE_COUNT, 0,
                                       nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint refCount = 0;
  ASSERT_SUCCESS(clGetCommandQueueInfo(queue, CL_QUEUE_REFERENCE_COUNT, size,
                                       &refCount, nullptr));
  ASSERT_EQ(1u, refCount);
}

TEST_F(clGetCommandQueueInfoTest, QueuePropertiesProfiling) {
  size_t size = 0;
  ASSERT_SUCCESS(
      clGetCommandQueueInfo(queue, CL_QUEUE_PROPERTIES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_command_queue_properties), size);
  cl_command_queue_properties properties = 0;
  ASSERT_SUCCESS(clGetCommandQueueInfo(queue, CL_QUEUE_PROPERTIES, size,
                                       &properties, nullptr));
  ASSERT_TRUE(CL_QUEUE_PROFILING_ENABLE & properties);
}

TEST_F(clGetCommandQueueInfoTest, QueuePropertiesOutOfOrderExec) {
  const cl_command_queue_properties deviceOutOfOrder =
      CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE & deviceSupportedProperties;

  size_t size = 0;
  ASSERT_SUCCESS(
      clGetCommandQueueInfo(queue, CL_QUEUE_PROPERTIES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_command_queue_properties), size);
  cl_command_queue_properties properties = 0;
  ASSERT_SUCCESS(clGetCommandQueueInfo(queue, CL_QUEUE_PROPERTIES, size,
                                       &properties, nullptr));
  ASSERT_EQ(deviceOutOfOrder,
            CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE & properties);
}

TEST_F(clGetCommandQueueInfoTest, ReturnBufferSizeTooSmall) {
  size_t param_value = 0;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetCommandQueueInfo(queue, CL_QUEUE_CONTEXT, 1, &param_value, nullptr));
}

#if defined(CL_VERSION_3_0)
TEST_F(clGetCommandQueueInfoTest, QueuePropertiesArray) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }
  size_t size;
  EXPECT_SUCCESS(clGetCommandQueueInfo(queue, CL_QUEUE_PROPERTIES_ARRAY, 0,
                                       nullptr, &size));
  ASSERT_EQ(size, sizeof(cl_command_queue_properties) * 3);
  std::array<cl_command_queue_properties, 3> properties;
  ASSERT_SUCCESS(clGetCommandQueueInfo(queue, CL_QUEUE_PROPERTIES_ARRAY, size,
                                       properties.data(), nullptr));
  EXPECT_EQ(properties[0], CL_QUEUE_PROPERTIES);
  EXPECT_EQ(properties[1], deviceSupportedProperties);
  EXPECT_EQ(properties[2], 0);
}

TEST_F(clGetCommandQueueInfoTest, QueuePropertiesArrayEmpty) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  cl_int errcode;
  cl_command_queue queue_with_no_properties =
      clCreateCommandQueueWithProperties(context, device, nullptr, &errcode);

  size_t size;
  EXPECT_SUCCESS(clGetCommandQueueInfo(
      queue_with_no_properties, CL_QUEUE_PROPERTIES_ARRAY, 0, nullptr, &size));
  EXPECT_EQ(size, 0);

  ASSERT_SUCCESS(clReleaseCommandQueue(queue_with_no_properties));
}

TEST_F(clGetCommandQueueInfoTest, QueueSizeInvalidCommandQueue) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }
  cl_device_device_enqueue_capabilities device_enqueue_capabilities;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES,
                                 sizeof(device_enqueue_capabilities),
                                 &device_enqueue_capabilities, nullptr));
  // If device side enqueue is supported, skip this negative test.
  if (device_enqueue_capabilities != 0) {
    GTEST_SKIP();
  }
  size_t size;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clGetCommandQueueInfo(queue, CL_QUEUE_SIZE, 0, nullptr, &size));
}

TEST_F(clGetCommandQueueInfoTest, QueueDeviceDefaultSuccess) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }
  size_t size;
  ASSERT_SUCCESS(
      clGetCommandQueueInfo(queue, CL_QUEUE_DEVICE_DEFAULT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_command_queue), size);
  cl_command_queue commandQueue;
  ASSERT_SUCCESS(clGetCommandQueueInfo(
      queue, CL_QUEUE_DEVICE_DEFAULT, sizeof(commandQueue),
      static_cast<void *>(&commandQueue), nullptr));
}

TEST_F(clGetCommandQueueInfoTest, QueueDeviceDeviceInvalidValue) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }
  cl_command_queue commandQueue;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetCommandQueueInfo(queue, CL_QUEUE_DEVICE_DEFAULT,
                            sizeof(commandQueue) - 1,
                            static_cast<void *>(&commandQueue), nullptr));
}
#endif
