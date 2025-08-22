#include <stack>

#include "Common.h"

class FuzzTest : public ucl::CommandQueueTest {
protected:
  void TearDown() override {
    for (size_t i = 0; i < buffers.size(); i++) {
      clReleaseMemObject(buffers[i]);
      while (!eventStacks[i].empty()) {
        clReleaseEvent(eventStacks[i].top());
        eventStacks[i].pop();
      }
    }
    CommandQueueTest::TearDown();
  }

  enum command_t { CREATE_BUFFER, READ_BUFFER, WRITE_BUFFER };

  std::vector<cl_mem> buffers;
  std::vector<std::stack<cl_event>> eventStacks;
  std::vector<std::vector<cl_int>> hostBuffers;
};

// See CA-1878 to enable
TEST_F(FuzzTest, DISABLED_ReadAndWriteTest) {
  const std::array<command_t, 120> commands{
      CREATE_BUFFER, READ_BUFFER,  CREATE_BUFFER, WRITE_BUFFER, WRITE_BUFFER,
      WRITE_BUFFER,  WRITE_BUFFER, WRITE_BUFFER,  WRITE_BUFFER, WRITE_BUFFER,
      WRITE_BUFFER,  WRITE_BUFFER, WRITE_BUFFER,  WRITE_BUFFER, WRITE_BUFFER,
      WRITE_BUFFER,  WRITE_BUFFER, WRITE_BUFFER,  WRITE_BUFFER, WRITE_BUFFER,
      WRITE_BUFFER,  READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  WRITE_BUFFER,
      WRITE_BUFFER,  READ_BUFFER,  WRITE_BUFFER,  WRITE_BUFFER, WRITE_BUFFER,
      READ_BUFFER,   WRITE_BUFFER, READ_BUFFER,   READ_BUFFER,  WRITE_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   WRITE_BUFFER, READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  WRITE_BUFFER,  READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  READ_BUFFER,   READ_BUFFER,  READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  WRITE_BUFFER,  WRITE_BUFFER, READ_BUFFER,
      READ_BUFFER,   READ_BUFFER,  WRITE_BUFFER,  READ_BUFFER,  WRITE_BUFFER};

  const std::array<size_t, 120> buffer_ids{
      0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0,
      1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1};

  const std::array<cl_bool, 120> blockings{
      0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
      1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};

  const std::array<size_t, 120> offsets{
      0,   28,  0,   28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,
      28,  28,  28,  28,  28,  28,  0,   0,   0,   280, 128, 128, 128, 128, 0,
      120, 504, 0,   504, 340, 340, 340, 340, 340, 340, 340, 340, 340, 340, 340,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   160, 0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   340,
      340, 340, 340, 340, 340, 340, 340, 504, 80,  80,  80,  0,   340, 340, 0};

  const std::array<size_t, 120> sizes{
      1024, 32,  1024, 32,  32, 32,  32,  32,  32,  32,  32,  32,  32,  32,
      32,   32,  32,   32,  32, 32,  32,  4,   4,   800, 648, 648, 648, 648,
      648,  4,   120,  504, 4,  504, 176, 176, 176, 176, 176, 176, 176, 176,
      176,  176, 176,  4,   4,  4,   4,   4,   4,   4,   4,   4,   4,   4,
      4,    4,   4,    4,   4,  4,   4,   4,   4,   4,   4,   32,  4,   4,
      4,    4,   4,    4,   4,  4,   4,   4,   4,   4,   4,   4,   4,   4,
      4,    4,   4,    4,   4,  4,   4,   4,   4,   4,   4,   4,   4,   4,
      4,    4,   4,    4,   4,  4,   176, 176, 176, 176, 176, 208, 176, 176,
      504,  84,  84,   84,  4,  176, 176, 4};

  const std::array<cl_uint, 120> num_events_in_wait_lists{
      0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  for (size_t i = 0; i < commands.size(); i++) {
    switch (commands[i]) {
    case CREATE_BUFFER: {
      cl_int errorCode;
      buffers.push_back(clCreateBuffer(context, CL_MEM_READ_WRITE, sizes[i],
                                       NULL, &errorCode));
      eventStacks.push_back(std::stack<cl_event>());
      EXPECT_TRUE(buffers.back());
      ASSERT_SUCCESS(errorCode);
      break;
    }
    case READ_BUFFER: {
      const cl_mem buffer = buffers[buffer_ids[i]];

      const cl_bool blocking = blockings[i];
      const size_t offset = offsets[i];
      const size_t size = sizes[i];

      hostBuffers.push_back(std::vector<cl_int>(sizes[i]));

      const cl_uint num_events_in_wait_list = num_events_in_wait_lists[i];
      const cl_event *event_wait_list = num_events_in_wait_list == 1
                                            ? &eventStacks[buffer_ids[i]].top()
                                            : NULL;
      cl_event event;

      ASSERT_SUCCESS(clEnqueueReadBuffer(
          command_queue, buffer, blocking, offset, size,
          hostBuffers.back().data(), num_events_in_wait_list, event_wait_list,
          &event));
      eventStacks[buffer_ids[i]].push(event);
      break;
    }
    case WRITE_BUFFER: {
      const cl_mem buffer = buffers[buffer_ids[i]];

      const cl_bool blocking = blockings[i];
      const size_t offset = offsets[i];
      const size_t size = sizes[i];

      hostBuffers.push_back(std::vector<cl_int>(sizes[i]));

      const cl_uint num_events_in_wait_list = num_events_in_wait_lists[i];
      const cl_event *event_wait_list = num_events_in_wait_list == 1
                                            ? &eventStacks[buffer_ids[i]].top()
                                            : NULL;
      cl_event event;

      ASSERT_SUCCESS(clEnqueueWriteBuffer(
          command_queue, buffer, blocking, offset, size,
          hostBuffers.back().data(), num_events_in_wait_list, event_wait_list,
          &event));
      eventStacks[buffer_ids[i]].push(event);
    }
    }
  }

  // Ensure all work is complete before finishing test.
  ASSERT_SUCCESS(clFinish(command_queue));
}
