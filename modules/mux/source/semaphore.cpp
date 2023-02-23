// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "mux/select.h"
#include "mux/utils/id.h"
#include "tracer/tracer.h"

mux_result_t muxCreateSemaphore(mux_device_t device,
                                mux_allocator_info_t allocator_info,
                                mux_semaphore_t *out_semaphore) {
  tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (nullptr == out_semaphore) {
    return mux_error_null_out_parameter;
  }

  mux_result_t error =
      muxSelectCreateSemaphore(device, allocator_info, out_semaphore);

  if (mux_success == error) {
    mux::setId<mux_object_id_semaphore>(device->info->id, *out_semaphore);
  }

  return error;
}

void muxDestroySemaphore(mux_device_t device, mux_semaphore_t semaphore,
                         mux_allocator_info_t allocator_info) {
  tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return;
  }

  if (mux::objectIsInvalid(semaphore)) {
    return;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return;
  }

  muxSelectDestroySemaphore(device, semaphore, allocator_info);
}

mux_result_t muxResetSemaphore(mux_semaphore_t semaphore) {
  tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(semaphore)) {
    return mux_error_invalid_value;
  }

  return muxSelectResetSemaphore(semaphore);
}
