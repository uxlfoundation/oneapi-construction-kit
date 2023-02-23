// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include "mux/select.h"
#include "mux/utils/id.h"
#include "tracer/tracer.h"

mux_result_t muxCreateFence(mux_device_t device,
                            mux_allocator_info_t allocator_info,
                            mux_fence_t *out_fence) {
  tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (nullptr == out_fence) {
    return mux_error_null_out_parameter;
  }

  auto error = muxSelectCreateFence(device, allocator_info, out_fence);

  if (mux_success == error) {
    mux::setId<mux_object_id_fence>(device->info->id, *out_fence);
  }
  return error;
}

void muxDestroyFence(mux_device_t device, mux_fence_t fence,
                     mux_allocator_info_t allocator_info) {
  tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return;
  }

  if (mux::objectIsInvalid(fence)) {
    return;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return;
  }

  muxSelectDestroyFence(device, fence, allocator_info);
}

mux_result_t muxResetFence(mux_fence_t fence) {
  tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(fence)) {
    return mux_error_invalid_value;
  }

  return muxSelectResetFence(fence);
}
