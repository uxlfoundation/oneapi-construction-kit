// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "ur/event.h"

#include "ur/device.h"
#include "ur/mux.h"
#include "ur/platform.h"
#include "ur/queue.h"

ur_event_handle_t_::~ur_event_handle_t_() {
  muxDestroyFence(queue->device->mux_device, mux_fence,
                  queue->device->platform->mux_allocator_info);
  muxDestroySemaphore(queue->device->mux_device, mux_semaphore,
                      queue->device->platform->mux_allocator_info);
}

cargo::expected<ur_event_handle_t, ur_result_t> ur_event_handle_t_::create(
    ur_queue_handle_t queue) {
  mux_fence_t mux_fence;
  if (auto error = muxCreateFence(queue->device->mux_device,
                                  queue->device->platform->mux_allocator_info,
                                  &mux_fence)) {
    return cargo::make_unexpected(ur::resultFromMux(error));
  }
  mux_semaphore_t mux_semaphore;
  if (auto error = muxCreateSemaphore(
          queue->device->mux_device,
          queue->device->platform->mux_allocator_info, &mux_semaphore)) {
    return cargo::make_unexpected(ur::resultFromMux(error));
  }

  auto event =
      std::make_unique<ur_event_handle_t_>(queue, mux_fence, mux_semaphore);
  if (!event) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }
  return event.release();
}

bool ur_event_handle_t_::wait() {
  return muxTryWait(queue->mux_queue, UINT64_MAX, mux_fence) == mux_success;
}

UR_APIEXPORT ur_result_t UR_APICALL urEventRelease(ur_event_handle_t event) {
  if (!event) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::release(event);
}

UR_APIEXPORT ur_result_t UR_APICALL
urEventWait(uint32_t numEvents, const ur_event_handle_t *eventList) {
  if (numEvents == 0) {
    return UR_RESULT_ERROR_INVALID_VALUE;
  }

  if (eventList == nullptr) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  for (uint32_t i = 0; i < numEvents; i++) {
    auto event = eventList[i];
    if (event == nullptr) {
      return UR_RESULT_ERROR_INVALID_EVENT;
    }
    if (!event->wait()) {
      return UR_RESULT_ERROR_INVALID_EVENT;
    }
  }
  return UR_RESULT_SUCCESS;
}
