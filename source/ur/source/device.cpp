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

#include "ur/device.h"

#include <algorithm>

#include "ur/info.h"
#include "ur/platform.h"

ur_device_handle_t_::ur_device_handle_t_(
    ur_platform_handle_t platform, mux_device_t mux_device,
    const compiler::Info *compiler_info,
    std::unique_ptr<compiler::Target> target,
    compiler::spirv::DeviceInfo spv_device_info)
    : platform{platform},
      mux_device{mux_device},
      compiler_info(compiler_info),
      target(std::move(target)),
      spv_device_info(std::move(spv_device_info)) {}

ur_device_handle_t_::~ur_device_handle_t_() {
  muxDestroyDevice(mux_device, platform->mux_allocator_info);
}

bool ur_device_handle_t_::supportsHostAllocations() const {
  const auto device_info = mux_device->info;
  const bool can_access_host = (0 != (device_info->allocation_capabilities &
                                      mux_allocation_capabilities_cached_host));
  const bool ptr_widths_match = (0 != (device_info->address_capabilities &
#if INTPTR_MAX == INT64_MAX
                                       mux_address_capabilities_bits64)
#elif INTPTR_MAX == INT32_MAX
                                       mux_address_capabilities_bits32)
#else
#error Unsupported pointer size
#endif
  );
  return can_access_host && ptr_widths_match;
}

UR_APIEXPORT ur_result_t UR_APICALL urDeviceGet(ur_platform_handle_t hPlatform,
                                                ur_device_type_t DevicesType,
                                                uint32_t NumEntries,
                                                ur_device_handle_t *phDevices,
                                                uint32_t *pNumDevices) {
  if (!hPlatform) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (hPlatform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }
  if (phDevices && NumEntries == 0) {
    return UR_RESULT_ERROR_INVALID_SIZE;
  }

  const auto nDevicesToReturn =
      std::min(NumEntries, static_cast<uint32_t>(hPlatform->devices.size()));

  cargo::small_vector<ur_device_handle_t, 4> devices;
  if (devices.resize(nDevicesToReturn)) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }
  for (size_t index = 0; index < nDevicesToReturn; index++) {
    devices[index] = &hPlatform->devices[index];
  }

  switch (DevicesType) {
    case UR_DEVICE_TYPE_DEFAULT:
      [[fallthrough]];
    case UR_DEVICE_TYPE_ALL:
      if (pNumDevices) {
        *pNumDevices = hPlatform->devices.size();
      }
      if (phDevices) {
        std::copy(devices.begin(), devices.end(), phDevices);
      }
      break;

    case UR_DEVICE_TYPE_GPU: {
      auto isGPU = [](ur_device_handle_t device) {
        switch (device->mux_device->info->device_type) {
          case mux_device_type_gpu_integrated:
          case mux_device_type_gpu_discrete:
          case mux_device_type_gpu_virtual:
            return true;
          default:
            return false;
        }
      };
      if (pNumDevices) {
        *pNumDevices = std::count_if(devices.begin(), devices.end(), isGPU);
      }
      if (phDevices) {
        std::copy_if(devices.begin(), devices.end(), phDevices, isGPU);
      }
    } break;

    case UR_DEVICE_TYPE_CPU: {
      auto isCPU = [](ur_device_handle_t device) {
        return device->mux_device->info->device_type == mux_device_type_cpu;
      };
      if (pNumDevices) {
        *pNumDevices = std::count_if(devices.begin(), devices.end(), isCPU);
      }
      if (phDevices) {
        std::copy_if(devices.begin(), devices.end(), phDevices, isCPU);
      }
    } break;

    case UR_DEVICE_TYPE_FPGA:
      [[fallthrough]];
    case UR_DEVICE_TYPE_MCA:
      [[fallthrough]];
    case UR_DEVICE_TYPE_VPU:
      if (pNumDevices) {
        *pNumDevices = 0;
      }
      break;

    default:
      // TODO: mux_device_type_accelerator
      // TODO: mux_device_type_custom
      return UR_RESULT_ERROR_INVALID_ENUMERATION;
  }

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urDeviceGetInfo(ur_device_handle_t hDevice,
                                                    ur_device_info_t infoType,
                                                    size_t propSize,
                                                    void *pDeviceInfo,
                                                    size_t *pPropSizeRet) {
  if (!hDevice) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (!ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }
  switch (infoType) {
    // TODO: Implement the other 93 enumerations.
    case UR_DEVICE_INFO_NAME:
      return ur::setInfo<const char *>(propSize, pDeviceInfo,
                                       hDevice->mux_device->info->device_name,
                                       pPropSizeRet);
    case UR_DEVICE_INFO_COMPILER_AVAILABLE:
      // We currently only support online compilation.
      return ur::setInfo<bool>(propSize, pDeviceInfo, true, pPropSizeRet);
    case UR_DEVICE_INFO_HOST_UNIFIED_MEMORY:
      return ur::setInfo<bool>(propSize, pDeviceInfo,
                               hDevice->supportsHostAllocations(),
                               pPropSizeRet);
    case UR_DEVICE_INFO_IL_VERSION:
      // TODO: To be implemented
      return ur::setInfo<const char *>(propSize, pDeviceInfo, "SPIR-V_1.6",
                                       pPropSizeRet);
    case UR_DEVICE_INFO_ADDRESS_BITS:
      if (hDevice->mux_device->info->address_capabilities &
          mux_address_capabilities_bits32) {
        return ur::setInfo<uint32_t>(propSize, pDeviceInfo,
                                     static_cast<uint32_t>(32), pPropSizeRet);
      } else if (hDevice->mux_device->info->address_capabilities &
                 mux_address_capabilities_bits64) {
        return ur::setInfo<uint32_t>(propSize, pDeviceInfo,
                                     static_cast<uint32_t>(64), pPropSizeRet);
      } else {
        return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
      }
    default:
      return UR_RESULT_ERROR_INVALID_ENUMERATION;
  }
}
