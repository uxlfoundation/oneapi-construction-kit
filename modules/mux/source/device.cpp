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

#include <algorithm>
#include <array>
#include <cassert>
#include <mutex>

#include "mux/select.h"
#include "mux/utils/id.h"
#include "tracer/tracer.h"

namespace {
mux_target_id_t makeTargetDeviceId(uint64_t targetIndex, uint64_t deviceIndex) {
  // mux_target_id_* is targetIndex + 1, deviceIndex shifts up 8 bits
  return (targetIndex + 1) | (deviceIndex << 8);
}

uint64_t getTargetIndex(mux_id_t id) {
  // deviceIndex is masked out, targetIndex is mux_target_id* - 1
  return (mux_target_id_device_mask & id) - 1;
}
};  // namespace

mux_result_t muxGetDeviceInfos(uint32_t device_types,
                               uint64_t device_infos_length,
                               mux_device_info_t *out_device_infos,
                               uint64_t *out_device_infos_length) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (0 == device_types) {
    return mux_error_invalid_value;
  }

  if ((nullptr == out_device_infos) && (nullptr == out_device_infos_length)) {
    return mux_error_null_out_parameter;
  }

  if ((0 < device_infos_length) && (nullptr == out_device_infos)) {
    return mux_error_null_out_parameter;
  }

  auto targetGetDeviceInfosHooks = muxGetGetDeviceInfosHooks();
  // Check that all hooks are valid, if they are not this is an internal error.
  for (uint64_t index = 0; index < mux_target_count; index++) {
    assert(nullptr != targetGetDeviceInfosHooks[index] &&
           "muxGetDeviceInfos_t is null");
  }

  // On first invocation we must initialize all the ID's for each device info
  // in each target, this is required so that future calls to
  // muxGetDeviceInfos with different values of device_types will not result
  // in uninitialized ID's.
  static std::once_flag deviceInfosInitialized;
  mux_result_t error = mux_success;
  std::call_once(deviceInfosInitialized, [&]() {
    for (uint64_t targetIndex = 0; targetIndex < mux_target_count;
         targetIndex++) {
      auto targetGetDeviceInfos = targetGetDeviceInfosHooks[targetIndex];

      uint64_t devicesLength = 0;
      if ((error = targetGetDeviceInfos(mux_device_type_all, 0, nullptr,
                                        &devicesLength))) {
        return;
      }

      // Stack allocate space for 16 target devices, this should be more than
      // enough but if this limit is ever reached, increase the size of the
      // array to fit the appropriate number of devices.
      std::array<mux_device_info_t, 16> deviceInfos;
      assert(devicesLength <= deviceInfos.size() &&
             "mux target has more than 16 devices, increase array size");

      if ((error = (targetGetDeviceInfos(mux_device_type_all, devicesLength,
                                         deviceInfos.data(), nullptr)))) {
        return;
      }

      for (uint64_t deviceIndex = 0; deviceIndex < devicesLength;
           deviceIndex++) {
        deviceInfos[deviceIndex]->id = mux::makeId(
            makeTargetDeviceId(targetIndex, deviceIndex), mux_object_id_device);
      }
    }
  });
  if (error) {
    return error;
  }

  // Determine the number of devices per target and the total number of all
  // devices for all targets.
  std::array<uint64_t, mux_target_count> numDevicesPerTarget;
  uint64_t numDevices = 0;
  for (uint64_t targetIndex = 0; targetIndex < mux_target_count;
       targetIndex++) {
    auto targetGetDeviceInfos = targetGetDeviceInfosHooks[targetIndex];
    if (auto error = targetGetDeviceInfos(device_types, 0, nullptr,
                                          &numDevicesPerTarget[targetIndex])) {
      return error;
    }
    numDevices += numDevicesPerTarget[targetIndex];
  }

  if ((numDevices != device_infos_length) && (nullptr != out_device_infos)) {
    return mux_error_invalid_value;
  }

  if (nullptr != out_device_infos_length) {
    *out_device_infos_length = numDevices;
  }

  if (nullptr != out_device_infos) {
    // Zero the array point mux_device_info_t pointers so that if we get a
    // failure later we can destroy the already created devices.
    std::fill_n(out_device_infos, device_infos_length, nullptr);

    uint64_t outOffset = 0;
    for (uint64_t targetIndex = 0; targetIndex < mux_target_count;
         targetIndex++) {
      auto targetGetDeviceInfos = targetGetDeviceInfosHooks[targetIndex];
      if (targetGetDeviceInfos(device_types, numDevicesPerTarget[targetIndex],
                               out_device_infos + outOffset, nullptr)) {
        return mux_error_device_entry_hook_failed;
      }
      outOffset += numDevicesPerTarget[targetIndex];
    }
  }

  return mux_success;
}

mux_result_t muxCreateDevices(uint64_t devices_length,
                              mux_device_info_t *device_infos,
                              mux_allocator_info_t allocator_info,
                              mux_device_t *out_devices) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (nullptr == out_devices) {
    return mux_error_null_out_parameter;
  }

  if (0 == devices_length) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  auto targetCreateDevicesHooks = muxGetCreateDevicesHooks();
  // Check that all hooks are valid, if they are not this is an internal error.
  for (uint64_t index = 0; index < mux_target_count; index++) {
    assert(nullptr != targetCreateDevicesHooks[index] &&
           "muxCreateDevices_t hook is null");
  }

  // Zero the array point mux_device_t pointers so that if we get a
  // failure later we can destroy the already created devices.
  std::fill_n(out_devices, devices_length, nullptr);

  // Closure that is used to clean up created devices when there was a failure.
  auto cleanup_devices = [&] {
    for (uint32_t index = 0; index < devices_length; index++) {
      // Because we zero'ed the array of mux_device_t's before, we can
      // check for successfully created devices here.
      if (out_devices[index]) {
        muxDestroyDevice(out_devices[index], allocator_info);
      }
    }
  };

  // Create all devices for all targets, this requires inspecting the ID's of
  // the device infos to determine the range of devices being returned which
  // should be passed to the appropriate targetCreateDevices.
  for (uint64_t deviceFirstIndex = 0; deviceFirstIndex < devices_length;) {
    // Ensure the device info has an ID.
    if (device_infos[deviceFirstIndex]->id == 0) {
      // invalid device info, destroy all previously created devices.
      cleanup_devices();
      return mux_error_feature_unsupported;
    }

    const auto targetIndex = getTargetIndex(device_infos[deviceFirstIndex]->id);
    auto deviceEndIndex = deviceFirstIndex;
    while (deviceEndIndex < devices_length &&
           targetIndex == getTargetIndex(device_infos[deviceEndIndex]->id)) {
      deviceEndIndex++;
    }
    // Get the actual index by shifting off device id from the mux_target_id
    auto targetCreateDevices = targetCreateDevicesHooks[targetIndex];
    if (targetCreateDevices(deviceEndIndex - deviceFirstIndex,
                            &device_infos[deviceFirstIndex], allocator_info,
                            &out_devices[deviceFirstIndex])) {
      // targetCreateDevices failed, destroy all previously created devices.
      cleanup_devices();
      return mux_error_device_entry_hook_failed;
    }
    deviceFirstIndex = deviceEndIndex;
  }

  for (uint64_t deviceIndex = 0; deviceIndex < devices_length; deviceIndex++) {
    // Assign the device the same ID as the device info, this allows
    // select.h to select entry points which take mux_device_t as the
    // first argument.
    out_devices[deviceIndex]->id = device_infos[deviceIndex]->id;

    // Iterate over every type of queue, and every queue of that type for
    // this device.  We then 'get' that queue so as we can set the ID on it.
    // We can do this here because although most objects are 'created' queues
    // are not, we are merely 'getting' them from the device.  The reason
    // that we need to do this here is that muxGetQueue must be thread-safe,
    // but this function (muxCreateDevices) is not.  Therefore if we do
    // setId in muxGetQueue we have a data-race because two threads might be
    // setting the value at once, but here we do not because
    // muxCreateDevices should never be called from two threads at once.
    for (unsigned queueTypeIndex = 0; queueTypeIndex < mux_queue_type_total;
         queueTypeIndex++) {
      const uint32_t numQueues =
          out_devices[deviceIndex]->info->queue_types[queueTypeIndex];
      for (uint32_t queueIndex = 0; queueIndex < numQueues; queueIndex++) {
        mux_queue_t queue;
        if (auto error =
                muxGetQueue(out_devices[deviceIndex],
                            static_cast<mux_queue_type_e>(queueTypeIndex),
                            queueIndex, &queue)) {
          // If we failed to create any of the queues for any device then
          // something went wrong.  We can't continue because we potentially
          // have a device queue without an ID, so destroy the devices and
          // return an error code.
          cleanup_devices();
          return error;
        }
        mux::setId<mux_object_id_queue>(out_devices[deviceIndex]->id, queue);
      }
    }
  }

  return mux_success;
}

void muxDestroyDevice(mux_device_t device,
                      mux_allocator_info_t allocator_info) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return;
  }

  muxSelectDestroyDevice(device, allocator_info);
}
