// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <common.h>
#include <algorithm>

TEST_F(MultiDeviceCommandQueue, Info) {
  std::vector<cl_device_id> queueDevices;
  for (auto command_queue : command_queues) {
    cl_context queueContext;
    ASSERT_EQ(CL_SUCCESS, clGetCommandQueueInfo(command_queue, CL_QUEUE_CONTEXT,
                                                sizeof(cl_context),
                                                &queueContext, nullptr));
    ASSERT_EQ(context, queueContext)
        << "The command queue was created with a different context.\n";
    cl_device_id queueDevice;
    ASSERT_EQ(CL_SUCCESS, clGetCommandQueueInfo(command_queue, CL_QUEUE_DEVICE,
                                                sizeof(cl_device_id),
                                                &queueDevice, nullptr));
    ASSERT_EQ(queueDevices.end(),
              std::find(queueDevices.begin(), queueDevices.end(), queueDevice))
        << "The command queue has the same device as another command queue but "
           "was created with a different device.\n";
    queueDevices.push_back(queueDevice);
  }
}

// Check that write then read on a buffer in a context with multiple device is
// consistent when the write and read are on queues to different devices.
TEST_F(MultiDeviceCommandQueue, WriteReadConsistency) {
  // Create a buffer to write and read on.
  constexpr size_t count = 1024;
  constexpr size_t size = sizeof(cl_uint) * 1024;
  cl_int error = !CL_SUCCESS;
  const auto buffer =
      clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &error);
  ASSERT_EQ(CL_SUCCESS, error);

  // Write some arbitrary value into the buffer in the first queue via a
  // blocking read command.
  const std::vector<cl_uint> input(count, 42);
  EXPECT_EQ(CL_SUCCESS, clEnqueueWriteBuffer(
                            command_queues[0], buffer, /* isBlocking */ CL_TRUE,
                            0, size, input.data(), 0, nullptr, nullptr));

  // Then try and read the buffer on the other queues, the buffer states should
  // be consitent across devices.
  for (unsigned i = 1; i < command_queues.size(); ++i) {
    std::vector<cl_uint> output(count, 0);
    EXPECT_EQ(CL_SUCCESS,
              clEnqueueReadBuffer(command_queues[i], buffer,
                                  /* isBlocking */ CL_TRUE, 0, size,
                                  output.data(), 0, nullptr, nullptr));
    EXPECT_EQ(input, output) << "Result on queue " << i << " is incorrect";
  }
  EXPECT_EQ(CL_SUCCESS, clReleaseMemObject(buffer));
}
