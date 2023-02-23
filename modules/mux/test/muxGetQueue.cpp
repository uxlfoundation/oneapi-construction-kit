// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxGetQueueTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxGetQueueTest);

TEST_P(muxGetQueueTest, Default) {
  mux_queue_t queue;
  for (uint32_t queue_index = 0;
       queue_index < device->info->queue_types[mux_queue_type_compute];
       queue_index++) {
    ASSERT_SUCCESS(
        muxGetQueue(device, mux_queue_type_compute, queue_index, &queue));
  }
}
