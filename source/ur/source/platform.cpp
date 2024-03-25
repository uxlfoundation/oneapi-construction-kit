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

#include "ur/platform.h"

#include <mutex>

#include "ur/config.h"
#include "ur/info.h"

// TODO: MEGA TODO!
// This is (almost) copy-pasta direct from:
// source/cl/source/binary/source/spirv.cpp, we should either unify this
// machinery or figure out how it should be different for UR.
static cargo::expected<compiler::spirv::DeviceInfo, cargo::result>
getSPIRVDeviceInfo(mux_device_info_t device_info) {
  static const std::array<const std::string, 1> supported_extensions = {
      {
          "SPV_KHR_no_integer_wrap_decoration",
      },
  };

  compiler::spirv::DeviceInfo spvDeviceInfo;

  if (auto result = spvDeviceInfo.capabilities.assign(
          {spv::CapabilityAddresses, spv::CapabilityFloat16Buffer,
           spv::CapabilityGroups, spv::CapabilityInt64, spv::CapabilityInt16,
           spv::CapabilityInt8, spv::CapabilityKernel, spv::CapabilityLinkage,
           spv::CapabilityVector16, spv::CapabilityKernelAttributesINTEL})) {
    return cargo::make_unexpected(result);
  }
  if (device_info->integer_capabilities & mux_integer_capabilities_64bit) {
    if (auto result =
            spvDeviceInfo.capabilities.push_back(spv::CapabilityInt64)) {
      return cargo::make_unexpected(result);
    }
  }

  if (device_info->image_support) {
    auto error_or = spvDeviceInfo.capabilities.insert(
        spvDeviceInfo.capabilities.end(), {
                                              spv::CapabilityImageBasic,
                                              spv::CapabilityLiteralSampler,
                                              spv::CapabilitySampled1D,
                                              spv::CapabilityImage1D,
                                              spv::CapabilitySampledBuffer,
                                              spv::CapabilityImageBuffer,
                                          });
    if (!error_or) {
      return cargo::make_unexpected(error_or.error());
    }
  }
  if (device_info->half_capabilities) {
    if (auto result =
            spvDeviceInfo.capabilities.push_back(spv::CapabilityFloat16)) {
      return cargo::make_unexpected(result);
    }
  }
  if (device_info->double_capabilities) {
    if (auto result =
            spvDeviceInfo.capabilities.push_back(spv::CapabilityFloat64)) {
      return cargo::make_unexpected(result);
    }
  }
  for (const std::string &extension : supported_extensions) {
    if (auto result = spvDeviceInfo.extensions.push_back(extension)) {
      return cargo::make_unexpected(result);
    }
  }
  spvDeviceInfo.memory_model = spv::MemoryModelOpenCL;
  if (device_info->address_capabilities & mux_address_capabilities_bits32) {
    spvDeviceInfo.addressing_model = spv::AddressingModelPhysical32;
  } else if (device_info->address_capabilities &
             mux_address_capabilities_bits64) {
    spvDeviceInfo.addressing_model = spv::AddressingModelPhysical64;
  }
  return {std::move(spvDeviceInfo)};
}

// TODO: More copy-pasta from cl::binary::detectBuiltinCapabilities().
static uint32_t detectBuiltinCapabilities(mux_device_info_t device_info) {
  uint32_t caps = 0;

  // Capabilities for doubles required for compliance
  // TODO: CA-882 Resolve how capabilities are checked
  const auto reqd_caps_fp64 = mux_floating_point_capabilities_denorm |
                              mux_floating_point_capabilities_inf_nan |
                              mux_floating_point_capabilities_rte |
                              mux_floating_point_capabilities_rtz |
                              mux_floating_point_capabilities_inf_nan |
                              mux_floating_point_capabilities_fma;

  // Capabilities for halfs required for compliance
  // TODO: CA-882 Resolve how capabilities are checked
  const auto reqd_caps_fp16_a = mux_floating_point_capabilities_rtz;
  const auto reqd_caps_fp16_b = mux_floating_point_capabilities_rte |
                                mux_floating_point_capabilities_inf_nan;

  // Bit width
  if ((device_info->address_capabilities & mux_address_capabilities_bits32) >
      0) {
    caps |= compiler::CAPS_32BIT;
  }

  // Doubles
  if ((device_info->double_capabilities & reqd_caps_fp64) == reqd_caps_fp64) {
    caps |= compiler::CAPS_FP64;
  }

  // Halfs
  if ((device_info->half_capabilities & reqd_caps_fp16_a) == reqd_caps_fp16_a ||
      (device_info->half_capabilities & reqd_caps_fp16_b) == reqd_caps_fp16_b) {
    caps |= compiler::CAPS_FP16;
  }

  return caps;
}

ur_platform_handle_t ur_platform_handle_t_::instance = nullptr;

UR_APIEXPORT ur_result_t UR_APICALL
urInit(ur_device_init_flags_t device_flags) {
  if (0x1 < device_flags) {
    return UR_RESULT_ERROR_INVALID_ENUMERATION;
  }

  static ur_result_t result;
  static ur_platform_handle_t_ platform;
  static std::once_flag once_flag;

  std::call_once(once_flag, [&]() {
    result = UR_RESULT_ERROR_UNINITIALIZED;
    uint64_t num_devices;
    if (muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &num_devices)) {
      result = UR_RESULT_ERROR_DEVICE_LOST;
      return;
    }
    cargo::small_vector<mux_device_info_t, 4> mux_device_infos;
    if (mux_device_infos.resize(num_devices)) {
      result = UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
      return;
    }
    if (muxGetDeviceInfos(mux_device_type_all, num_devices,
                          mux_device_infos.data(), nullptr)) {
      result = UR_RESULT_ERROR_DEVICE_LOST;
      return;
    }

    cargo::small_vector<mux_device_t, 4> mux_devices;
    if (mux_devices.resize(num_devices)) {
      result = UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
      return;
    }
    if (muxCreateDevices(num_devices, mux_device_infos.data(),
                         platform.mux_allocator_info, mux_devices.data())) {
      result = UR_RESULT_ERROR_DEVICE_LOST;
      return;
    }

    if (platform.devices.reserve(num_devices)) {
      result = UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
      return;
    }

    // TODO: Don't assume we need a compiler.
    auto compiler_library = compiler::loadLibrary();
    assert(compiler_library.has_value() &&
           "Error: Compiler library not present, we do not currently support "
           "an offline Unified Runtime");
    if (!compiler_library) {
      result = UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
      return;
    }
    platform.compiler_library = std::move(compiler_library.value());

    // TODO: Do this lazily (see CL's _cl_context::getCompilerContext()).
    auto compiler_context =
        compiler::createContext(platform.compiler_library.get());
    if (!compiler_context) {
      result = UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
      return;
    }
    platform.compiler_context = std::move(compiler_context);

    // We need to construct the devices after we've initialized the compiler
    // library because the devices will attempt to get a compiler for each
    // device from the library belonging to the platform.
    for (uint64_t index = 0; index < num_devices; index++) {
      auto compiler_info = compiler::getCompilerForDevice(
          platform.compiler_library.get(), mux_devices[index]->info);

      auto target = compiler_info->createTarget(platform.compiler_context.get(),
                                                compiler::NotifyCallbackFn{});
      if (!target) {
        result = UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
        return;
      }

      if (target->init(detectBuiltinCapabilities(mux_devices[index]->info)) !=
          compiler::Result::SUCCESS) {
        result = UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
        return;
      }

      auto const device_info = mux_devices[index]->info;
      auto spv_device_info = *getSPIRVDeviceInfo(device_info);

      if (platform.devices.emplace_back(&platform, mux_devices[index],
                                        compiler_info, std::move(target),
                                        std::move(spv_device_info))) {
        result = UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
        return;
      }
    }
    ur_platform_handle_t_::instance = &platform;
    result = UR_RESULT_SUCCESS;
  });

  return result;
}

UR_APIEXPORT ur_result_t UR_APICALL
urPlatformGet(uint32_t NumEntries, ur_platform_handle_t *phPlatforms,
              uint32_t *pNumPlatforms) {
  if (!ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }
  if (phPlatforms && NumEntries == 0) {
    return UR_RESULT_ERROR_INVALID_SIZE;
  }

  if (pNumPlatforms) {
    *pNumPlatforms = 1;
  }
  if (phPlatforms) {
    *phPlatforms = ur_platform_handle_t_::instance;
  }
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urPlatformGetInfo(
    ur_platform_handle_t hPlatform, ur_platform_info_t PlatformInfoType,
    size_t propSize, void *pPlatformInfo, size_t *pSizeRet) {
  if (!hPlatform) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (hPlatform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }
  switch (PlatformInfoType) {
    case UR_PLATFORM_INFO_NAME:
      return ur::setInfo<const char *>(propSize, pPlatformInfo,
                                       CA_UR_PLATFORM_NAME, pSizeRet);
    case UR_PLATFORM_INFO_VENDOR_NAME:
      return ur::setInfo<const char *>(propSize, pPlatformInfo,
                                       CA_UR_PLATFORM_VENDOR, pSizeRet);
    case UR_PLATFORM_INFO_VERSION:
      return ur::setInfo<const char *>(propSize, pPlatformInfo,
                                       CA_UR_PLATFORM_VERSION, pSizeRet);
    case UR_PLATFORM_INFO_EXTENSIONS:
      return ur::setInfo<const char *>(propSize, pPlatformInfo,
                                       CA_UR_PLATFORM_EXTENSIONS, pSizeRet);
    case UR_PLATFORM_INFO_PROFILE:
      return ur::setInfo<const char *>(propSize, pPlatformInfo,
                                       CA_UR_PLATFORM_PROFILE, pSizeRet);
    default:
      return UR_RESULT_ERROR_INVALID_ENUMERATION;
  }
}

UR_APIEXPORT ur_result_t UR_APICALL urTearDown(void *pParams) {
  // TODO: Reconsider this. The spec requires pParams be non-null but this seems
  // a bit much.
  if (!pParams) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }
  return UR_RESULT_SUCCESS;
}
