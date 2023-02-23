// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "mux/select.h"
#include "mux/utils/id.h"
#include "tracer/tracer.h"

mux_result_t muxCreateExecutable(mux_device_t device, const void *binary,
                                 uint64_t binary_length,
                                 mux_allocator_info_t allocator_info,
                                 mux_executable_t *out_executable) {
  tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (nullptr == binary) {
    return mux_error_invalid_value;
  }

  if (0 == binary_length) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (nullptr == out_executable) {
    return mux_error_null_out_parameter;
  }

  mux_result_t error = muxSelectCreateExecutable(
      device, binary, binary_length, allocator_info, out_executable);

  if (mux_success == error) {
    mux::setId<mux_object_id_executable>(device->id, *out_executable);
  }

  return error;
}

void muxDestroyExecutable(mux_device_t device, mux_executable_t executable,
                          mux_allocator_info_t allocator_info) {
  tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return;
  }

  if (mux::objectIsInvalid(executable)) {
    return;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return;
  }

  muxSelectDestroyExecutable(device, executable, allocator_info);
}
